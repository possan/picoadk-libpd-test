#include <stdio.h>
#include "project_config.h"

#ifdef USE_USB_MIDI
#if __has_include("bsp/board_api.h")
#include "bsp/board_api.h"
#else
#include "bsp/board.h"
#endif

#include "midi_input_usb.h"
#endif

#include "audio_subsystem.h"
#include "vult.h"
#include "picoadk_hw.h"

#include "hardware/exception.h"
#include <pico/bootrom.h>
#include "m0FaultDispatch.h"

#include <malloc.h>
#include "FreeRTOS.h"
#include <task.h>
#include <queue.h>

// #include "arduino_compat.h"

#include <pthread.h>
#include <z_libpd.h>
#include <z_print_util.h>
extern "C" {
#include <z_ringbuffer.h>
}
#include "g_canvas.h"

#define USE_DIN_MIDI 0
#define DEBUG_MIDI 0

// Set to 0 if you want to play notes via USB MIDI
#define PLAY_RANDOM_NOTES 1

audio_buffer_pool_t *ap;
Dsp_process_type ctx;

#ifdef USE_USB_MIDI
MIDIInputUSB usbmidi;
#endif

extern "C" {
    extern void __attribute__((used,naked)) HardFault_Handler(void);
}

#ifdef __cplusplus
extern "C"
{
#endif

    volatile long i2s_start;
    volatile long i2s_end;
    volatile long pd_start;
    volatile long pd_end;
    volatile int i2s_underruns = 0;
    volatile int i2s_counter = 0;
	ring_buffer *_inputRingBuffer; ///< input buffer
	ring_buffer *_outputRingBuffer; ///< output buffer
    int blocksize = 4;


    // This task prints the statistics about the running FreeRTOS tasks
    // and how long it takes for the I2S callback to run.
    void print_task(void *p)
    {
        char ptrTaskList[2048];
        while (1)
        {
            vTaskList(ptrTaskList);
            printf("\033[2J");
            printf("\033[0;0HTask\t\tState\tPrio\tStack\tNum\n%s\n", ptrTaskList);
            printf("======================================================\n");
            printf("B = Blocked, R = Ready, D = Deleted, S = Suspended\n");
            printf("Milliseconds since boot: %d\n", xTaskGetTickCount() * portTICK_PERIOD_MS);
            printf("pd render took %d uS\n", pd_end - pd_start);
            printf("i2s task took %d uS, %d underruns\n", i2s_end - i2s_start, i2s_underruns);
            watchdog_update();
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    // MIDI callbacks
    void note_on_callback(uint8_t note, uint8_t level, uint8_t channel)
    {
        if (level > 0)
        {
            Dsp_noteOn(ctx, note, level, channel);
            gpio_put(15, 1);
#if DEBUG_MIDI
            printf("note on (ch %d): %d %d\n", channel, note, level);
#endif
        }
        else
        {
            Dsp_noteOff(ctx, note, channel);
            gpio_put(15, 0);
#if DEBUG_MIDI
            printf("note off (ch %d): %d %d\n", channel, note, level);
#endif
        }
    }

    void note_off_callback(uint8_t note, uint8_t level, uint8_t channel)
    {
        Dsp_noteOff(ctx, note, channel);
        gpio_put(15, 0);
#if DEBUG_MIDI
        printf("note off (ch %d): %d %d\n", channel, note, level);
#endif
    }

    void cc_callback(uint8_t cc, uint8_t value, uint8_t channel)
    {
        Dsp_controlChange(ctx, cc, value, channel);
#if DEBUG_MIDI
        printf("cc (ch %d): %d %d\n", channel, cc, value);
#endif
    }

#ifdef USE_USB_MIDI
    // This task processes the USB MIDI input
    void usb_midi_task(void *pvParameters)
    {
        usbmidi.setCCCallback(cc_callback);
        usbmidi.setNoteOnCallback(note_on_callback);
        usbmidi.setNoteOffCallback(note_off_callback);

        while (1)
        {
            tud_task();
            usbmidi.process();
        }
    }
#endif
    // This task blinks the LEDs on GPIO 2-5
    void blinker_task(void *pvParameters)
    {
        // set gpio 2-5 and 15 as outputs
        for (int i = 2; i < 6; i++)
        {
            gpio_init(i);
            gpio_set_dir(i, GPIO_OUT);
        }

        while (1)
        {
            // chase leds on gpio 2-5
            for (int i = 2; i < 6; i++)
            {
                gpio_put(i, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                gpio_put(i, 0);
            }
        }
    }


    // This tasks generates random notes and plays them.
    // It is only used if PLAY_RANDOM_NOTES is set to 1.
    void play_task(void *pvParameters)
    {
        while (1)
        {
            uint8_t lydianScale[7] = {0, 2, 4, 6, 7, 9, 11};
            uint8_t noteArray[16];
            uint8_t velocityArray[16];
            bool restArray[16];
            uint8_t noteLengthArray[16];

            for (int i = 0; i < 16; i++)
            {
                uint8_t randomOctave = rand() % 3;
                noteArray[i] = (lydianScale[rand() % 7] + 60 - 12) + randomOctave * 12;
                velocityArray[i] = 64 + rand() % 63;
                restArray[i] = rand() % 2;
                noteLengthArray[i] = 50 + (rand() % 50);
            }

            uint8_t noteInterval = 100;

            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 16; j++)
                {
                    if (!restArray[j])
                    {
                        note_on_callback(noteArray[j], velocityArray[j], 0);
                        vTaskDelay(pdMS_TO_TICKS(noteInterval));
                        note_off_callback(noteArray[j], velocityArray[j], 0);
                        vTaskDelay(pdMS_TO_TICKS(noteInterval));
                    }
                    else
                    {
                        vTaskDelay(pdMS_TO_TICKS(noteInterval * 2));
                    }
                }
            }
        }
    }

    // This tasks generates random notes and plays them.
    // It is only used if PLAY_RANDOM_NOTES is set to 1.
    void render_task(void *pvParameters)
    {
        int last_cv0 = -1;
        int last_cv1 = -1;
        int last_cv2 = -1;
        int last_cv3 = -1;

        int agg_cv0[8] = { 0, };
        int agg_cv1[8] = { 0, };
        int agg_cv2[8] = { 0, };
        int agg_cv3[8] = { 0, };

        int agg_idx = 0;

        int count = 0;
        while (1)
        {
            t_float inbuf[64] = {0,};
            t_float outbuf[64] = {0, };

            uint32_t framesWritable = (uint32_t)rb_available_to_write(_outputRingBuffer) / (sizeof(float) * 1);
            if (framesWritable >= 64) {
                int cv0 = (adc128_read(0) + adc128_read(0) + adc128_read(0) + adc128_read(0)) / 4;
                // agg_cv0[agg_idx] = cv0;
                if (last_cv0 == -1) {
                    last_cv0 = cv0;
                }

                // cv0 = (agg_cv0[0] + agg_cv0[1] + agg_cv0[2] + agg_cv0[3]) / 4;
                if (abs(cv0 - last_cv0) > 32) {
                    printf("cv0 changed to %d\n", cv0);
                    last_cv0 = cv0;
                    libpd_start_message(1);
                    libpd_add_float(cv0);
                    libpd_finish_message("cv0", "float");
                }

                int cv1 = (adc128_read(1) + adc128_read(1) + adc128_read(1) + adc128_read(1)) / 4;
                if (last_cv1 == -1) {
                    last_cv1 = cv1;
                }

                // agg_cv1[agg_idx] = cv1;

                // cv1 = (agg_cv1[0] + agg_cv1[1] + agg_cv1[2] + agg_cv1[3]) / 4;
                if (abs(cv1 - last_cv1) > 32) {
                    printf("cv1 changed to %d\n", cv1);
                    last_cv1 = cv1;
                    libpd_start_message(1);
                    libpd_add_float(cv1);
                    libpd_finish_message("cv1", "float");
                }

                int cv2 = (adc128_read(2) + adc128_read(2) + adc128_read(2) + adc128_read(2)) / 4;
                if (last_cv2 == -1) {
                    last_cv2 = cv2;
                }
                // agg_cv2[agg_idx] = cv2;

                // cv2 = (agg_cv2[0] + agg_cv2[1] + agg_cv2[2] + agg_cv2[3]) / 4;
                if (abs(cv2 - last_cv2) > 32) {
                    printf("cv2 changed to %d\n", cv2);
                    last_cv2 = cv2;
                    libpd_start_message(1);
                    libpd_add_float(cv2);
                    libpd_finish_message("cv2", "float");
                }

                pd_start = to_us_since_boot(get_absolute_time());
                for(int k=0; k<4; k++) {
                    uint32_t framesWritable = (uint32_t)rb_available_to_write(_outputRingBuffer) / (sizeof(float) * 1);
                    if(framesWritable >= 64) {
                        libpd_process_float(1, inbuf, outbuf);
                        // for(int j=0; j<64; j++) {
                        //     outbuf[j] += (float)(rand() % 100) / 300.0;
                        // }
                        rb_write_to_buffer(_outputRingBuffer, 1, (char *)&outbuf,  (int)(blocksize * sizeof(float)));
                    }
                }
                pd_end = to_us_since_boot(get_absolute_time());
            }

            vTaskDelay(pdMS_TO_TICKS(1));
            agg_idx += 1;
            agg_idx %= 4;
            count ++;
        }
    }

uint32_t getTotalHeap(void) {
  extern char __StackLimit, __bss_end__;
  return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
  struct mallinfo m = mallinfo();
  return getTotalHeap() - m.uordblks;
}

void pdprint(const char *s)
{
  printf("%s", s);
}

void pdnoteon(int ch, int pitch, int vel)
{
  printf("noteon: %d %d %d\n", ch, pitch, vel);
}

// sed 's/;$/;\\/' foo.pd | sed 's/#N //'
static const char oscpatchfile[] =
  "#N canvas 0 50 450 300 12;\n\
  #X obj 190 104 loadbang;\n\
  #X msg 190 129; pd dsp 1;\n\
  #X obj 118 123 dac~ 1;\n\
  #X obj 118 98 osc~ 100;\n\
  #X connect 0 0 1 0;\n\
  #X connect 3 0 2 0;\n\
  ";

static const char westcoastpatchfile[] =
  "#N canvas 654 179 940 1030 12;\n\
#X floatatom 486 172 5 10 999 0 - - - 0;\n\
#X obj 323 692 mtof;\n\
#X msg 109 303 1;\n\
#X obj 32 853 *~;\n\
#X obj 32 910 cos~;\n\
#X obj 32 935 hip~ 5;\n\
#X obj 32 887 +~ 0.1;\n\
#X floatatom 68 648 0 0 0 0 - - - 0;\n\
#X floatatom 210 650 0 0 200 0 - - - 0;\n\
#X floatatom 133 650 0 0 999 0 - - - 0;\n\
#X floatatom 170 650 0 0 999 0 - - - 0;\n\
#X obj 34 719 *~ 0.01;\n\
#X obj 251 163 tgl 19 0 empty empty empty 0 -6 0 8 #dfdfdf #000000 #000000 0 1;\n\
#X floatatom 317 275 5 -48 120 0 - - - 0;\n\
#X obj 251 200 sel 0;\n\
#X msg 27 300 0;\n\
#X obj 417 204 metro 130;\n\
#X obj 323 721 osc~;\n\
#N canvas 121 146 915 638 adsr 0;\n\
#X obj 129 120 inlet;\n\
#X obj 438 160 inlet;\n\
#X obj 129 148 sel 0;\n\
#X obj 190 273 f \\$1;\n\
#X obj 495 160 inlet;\n\
#X obj 422 285 del \\$2;\n\
#X obj 595 456 line~;\n\
#X obj 446 313 f \\$4;\n\
#X obj 545 160 inlet;\n\
#X obj 600 160 inlet;\n\
#X obj 656 160 inlet;\n\
#X msg 129 179 stop;\n\
#X obj 596 315 pack 0 \\$5;\n\
#X obj 485 364 * \\$1;\n\
#X obj 595 486 outlet~;\n\
#X obj 446 338 * 0.01;\n\
#X obj 231 101 moses;\n\
#X obj 218 131 t b b;\n\
#X msg 152 299 0;\n\
#X obj 263 164 b;\n\
#X obj 190 298 pack f \\$2;\n\
#X obj 485 388 pack f \\$3;\n\
#X connect 0 0 2 0;\n\
#X connect 1 0 3 1;\n\
#X connect 1 0 13 1;\n\
#X connect 2 0 11 0;\n\
#X connect 2 0 12 0;\n\
#X connect 2 1 16 0;\n\
#X connect 3 0 20 0;\n\
#X connect 4 0 5 1;\n\
#X connect 4 0 20 1;\n\
#X connect 5 0 7 0;\n\
#X connect 6 0 14 0;\n\
#X connect 7 0 15 0;\n\
#X connect 8 0 21 1;\n\
#X connect 9 0 7 1;\n\
#X connect 10 0 12 1;\n\
#X connect 11 0 5 0;\n\
#X connect 12 0 6 0;\n\
#X connect 13 0 21 0;\n\
#X connect 15 0 13 0;\n\
#X connect 16 0 17 0;\n\
#X connect 16 1 19 0;\n\
#X connect 17 0 19 0;\n\
#X connect 17 1 18 0;\n\
#X connect 18 0 6 0;\n\
#X connect 19 0 3 0;\n\
#X connect 19 0 5 0;\n\
#X connect 20 0 6 0;\n\
#X connect 21 0 6 0;\n\
#X restore 34 695 pd adsr 10 5 50 50;\n\
#X f 30;\n\
#X obj 368 529 hsl 444 19 0 1 0 0 empty empty empty -2 -10 0 12 #fcfcfc #000000 #000000 0 1;\n\
#X obj 251 92 loadbang;\n\
#X obj 97 108 notein;\n\
#X obj 97 148 stripnote;\n\
#X obj 368 490 hsl 444 19 50 350 0 0 empty empty empty -2 -10 0 12 #fcfcfc #000000 #000000 0 1;\n\
#X msg 134 610 1;\n\
#X obj 25 225 sel 0;\n\
#X obj 33 973 dac~;\n\
#X msg 250 136 1;\n\
#X obj 363 394 line;\n\
#X obj 365 350 pack 0 633;\n\
#X msg 171 608 96;\n\
#X msg 344 121 \\; pd dsp 1;\n\
#X obj 479 393 line;\n\
#X obj 322 21 r cv0;\n\
#X obj 644 293 r cv2;\n\
#X obj 488 19 r cv1;\n\
#X obj 416 261 random 48;\n\
#X obj 300 318 + 6;\n\
#X obj 481 318 / 60;\n\
#X obj 364 316 + 64;\n\
#X obj 481 349 pack 0 533;\n\
#X obj 321 52 / 100;\n\
#X obj 320 80 int;\n\
#X obj 485 109 int;\n\
#X obj 488 48 / 100;\n\
#X obj 486 77 + 100;\n\
#X connect 0 0 16 1;\n\
#X connect 1 0 17 0;\n\
#X connect 2 0 18 0;\n\
#X connect 3 0 6 0;\n\
#X connect 4 0 5 0;\n\
#X connect 5 0 26 0;\n\
#X connect 5 0 26 1;\n\
#X connect 6 0 4 0;\n\
#X connect 7 0 18 1;\n\
#X connect 8 0 18 4;\n\
#X connect 9 0 18 2;\n\
#X connect 10 0 18 3;\n\
#X connect 10 0 18 5;\n\
#X connect 11 0 3 0;\n\
#X connect 12 0 14 0;\n\
#X connect 12 0 16 0;\n\
#X connect 13 0 37 1;\n\
#X connect 14 0 15 0;\n\
#X connect 15 0 18 0;\n\
#X connect 16 0 2 0;\n\
#X connect 16 0 36 0;\n\
#X connect 17 0 3 1;\n\
#X connect 18 0 11 0;\n\
#X connect 19 0 6 1;\n\
#X connect 20 0 27 0;\n\
#X connect 20 0 31 0;\n\
#X connect 20 0 30 0;\n\
#X connect 20 0 24 0;\n\
#X connect 21 0 22 0;\n\
#X connect 21 1 22 1;\n\
#X connect 22 0 1 0;\n\
#X connect 22 1 25 0;\n\
#X connect 23 0 7 0;\n\
#X connect 24 0 9 0;\n\
#X connect 25 0 15 0;\n\
#X connect 25 1 2 0;\n\
#X connect 27 0 12 0;\n\
#X connect 28 0 23 0;\n\
#X connect 29 0 28 0;\n\
#X connect 30 0 7 0;\n\
#X connect 30 0 10 0;\n\
#X connect 32 0 19 0;\n\
#X connect 33 0 41 0;\n\
#X connect 35 0 44 0;\n\
#X connect 36 0 37 0;\n\
#X connect 36 0 38 0;\n\
#X connect 36 0 39 0;\n\
#X connect 37 0 1 0;\n\
#X connect 38 0 40 0;\n\
#X connect 39 0 29 0;\n\
#X connect 40 0 32 0;\n\
#X connect 41 0 42 0;\n\
#X connect 42 0 13 0;\n\
#X connect 43 0 0 0;\n\
#X connect 44 0 45 0;\n\
#X connect 45 0 43 0;\n\
";

    void binbuf_evalfile_but_text(const char *patchtext)
    {
        t_binbuf *b = binbuf_new();
        glob_setfilename(0, gensym("main-patch"), gensym("."));
        binbuf_text(b, patchtext, strlen(patchtext));
        t_pd *bounda = gensym("#A")->s_thing, *boundn = s__N.s_thing;
        gensym("#A")->s_thing = 0;
        s__N.s_thing = &pd_canvasmaker;
        binbuf_eval(b, 0, 0, 0);
            /* avoid crashing if no canvas was created by binbuf eval */
        if (s__X.s_thing && *s__X.s_thing == canvas_class)
            canvas_initbang((t_canvas *)(s__X.s_thing)); /* JMZ*/
        gensym("#A")->s_thing = bounda;
        s__N.s_thing = boundn;
        glob_setfilename(0, &s_, &s_);
        binbuf_free(b);
    }

    t_pd *glob_evalfile_but_text(const char *patchtext){
        t_pd *x = 0, *boundx;
        int dspstate;
        boundx = s__X.s_thing; s__X.s_thing = 0;
        binbuf_evalfile_but_text(patchtext);
        while ((x != s__X.s_thing) && s__X.s_thing)
        {
            x = s__X.s_thing;
            vmess(x, gensym("pop"), "i", 1);
        }
        pd_vmess(x, gensym("loadbang"), "f", LB_LOAD);
        s__X.s_thing = boundx;
        return x;
    }

    int main(void)
    {
        // initialize the hardware
        picoadk_init();
        exception_set_exclusive_handler(HARDFAULT_EXCEPTION, HardFault_Handler);

        sleep_ms(2000);
        printf("memory: free heap %ld of %ld\n" ,getFreeHeap(), getTotalHeap());

        int srate = 44100;
        blocksize = libpd_blocksize();
        libpd_set_verbose(1);
        libpd_set_printhook(pdprint);
        libpd_set_noteonhook(pdnoteon);
        libpd_init();
        libpd_init_audio(1, 1, srate);
        printf("blocksize %d\n", blocksize);

        printf("memory: free heap %ld of %ld\n" ,getFreeHeap(), getTotalHeap());

        _inputRingBuffer = rb_create(sizeof(float) * 16 * blocksize);
        _outputRingBuffer = rb_create(sizeof(float) * 16 * blocksize);

        printf("\n*** before creating initial patch.\n");
        // glob_evalfile_but_text((const char *)&osctest2patch);
        glob_evalfile_but_text((const char *)&westcoastpatchfile);
        printf("\n*** after creating initial patch.\n\n");

        printf("Before benchmark\n");
        uint32_t T0 = time_us_32();

        t_float inbuf[64];
        t_float outbuf[64];
        for (int i = 0; i < srate / 64; i++) {
            memset(outbuf, 0, 64*sizeof(float));
            libpd_process_float(1, inbuf, outbuf);
        }

        uint32_t T1 = time_us_32();
        printf("generating 1 second of audio in %d hz took %d us \n", srate, T1-T0);

        for(int k=0; k<64; k+=4) {
            printf("  soundout[%d] = %f\n", k, outbuf[k]);
        }

        printf("memory: free heap %ld of %ld\n" ,getFreeHeap(), getTotalHeap());

        // Initialize Vult DSP context.
        // Dsp_process_init(ctx);
        // Dsp_default_init(ctx);
        // Dsp_default(ctx);

        // Initialize the audio subsystem
        ap = init_audio();

        // Create FreeRTOS Tasks for USB MIDI and printing statistics
#ifdef USE_USB_MIDI
        xTaskCreate(usb_midi_task, "USBMIDI", 4096, NULL, configMAX_PRIORITIES, NULL);
#endif
        xTaskCreate(print_task, "TASKLIST", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
        xTaskCreate(blinker_task, "BLINKER", 128, NULL, configMAX_PRIORITIES - 1, NULL);
#if PLAY_RANDOM_NOTES
        xTaskCreate(play_task, "PLAY", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
#endif
        xTaskCreate(render_task, "RENDER", 4096, NULL, configMAX_PRIORITIES - 1, NULL);

        // Start the scheduler.
        vTaskStartScheduler();

        // Idle loop.
        while (1)
        {
            ;
            ;
        }
    }


    // This fis the I2S callback function. It is called when the I2S subsystem
    // needs more audio data. It is called at a fixed rate of 48kHz.
    // The audio data is stored in the audio_buffer_t struct.
    void __not_in_flash_func(i2s_callback_func())
    {
        audio_buffer_t *buffer = take_audio_buffer(ap, false);
        if (buffer == NULL)
        {
            return;
        }
        int32_t *samples = (int32_t *)buffer->buffer->bytes;

        i2s_start = to_us_since_boot(get_absolute_time());

        // convert 12-bit adc value to 16-bit signed int
        // todo: use vult external function to read adcs instead
        // t_float inbuf[64] = {0,};
        t_float outbuf2[64] = {0, };

        if (_inputRingBuffer && _outputRingBuffer) {

    		//  uint32_t framesAvailable = (uint32_t)rb_available_to_read(_outputRingBuffer) / (sizeof(float) * 1);
            //  if (framesAvailable < 64 ){
            //      libpd_process_float(1, inbuf, outbuf);
            //      rb_write_to_buffer(_outputRingBuffer, 64 * sizeof(float), (char *)&outbuf);
            //  }

             uint32_t framesReadable = (uint32_t)rb_available_to_read(_outputRingBuffer) / (sizeof(float));
            if (framesReadable >= buffer->sample_count){
                rb_read_from_buffer(_outputRingBuffer, (char *)&outbuf2, buffer->max_sample_count*sizeof(float));
            } else {
                i2s_underruns  ++;
            }

            // for(int j=0; j<buffer->sample_count; j++) {
            //     outbuf2[j] = (float)(rand() % 200) / 200.0;
            // }
        }


        // We are filling the buffer with 32-bit samples (2 channels)
        for (uint i = 0; i < buffer->sample_count; i++)
        {
            fix16_t left_out = float_to_fix(outbuf2[i] * 1.0f);
            samples[i * 2 + 0] = fix16_to_int32(left_out);  // LEFT
            samples[i * 2 + 1] = fix16_to_int32(left_out); // RIGHT
        }

        i2s_end = to_us_since_boot(get_absolute_time());

        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(ap, buffer);
        i2s_counter ++;
        return;
    }

#ifdef __cplusplus
}
#endif
