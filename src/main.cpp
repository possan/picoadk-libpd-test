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

    volatile long dsp_start;
    volatile long dsp_end;

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
            printf("dsp task took %d uS\n", dsp_end - dsp_start);
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
  "canvas 0 50 450 300 12;\n\
  #X obj 190 104 loadbang;\n\
  #X msg 190 129; pd dsp 1;\n\
  #X obj 118 123 dac~ 1;\n\
  #X obj 118 98 osc~ 10;\n\
  #X connect 0 0 1 0;\n\
  #X connect 3 0 2 0;\n\
  ";

static const char westcoastpatchfile[] =
  "canvas 335 106 760 745 12;\
#X floatatom 619 49 5 10 999 0 - - - 0;\
#X obj 56 327 mtof;\
#X msg 30 172 1;\
#X obj 28 374 *~;\
#X obj 28 434 cos~;\
#X obj 28 459 hip~ 5;\
#X obj 28 411 +~ 0.1;\
#X floatatom 44 225 0 0 0 0 - - - 0;\
#X floatatom 169 225 0 0 200 0 - - - 0;\
#X floatatom 103 224 0 0 999 0 - - - 0;\
#X floatatom 136 224 0 0 999 0 - - - 0;\
#X obj 30 279 *~ 0.01;\
#X obj 453 63 tgl 19 0 empty empty empty 0 -6 0 8 #dfdfdf #000000 #000000 0 1;\
#X floatatom 502 153 5 -48 120 0 - - - 0;\
#X obj 279 97 sel 0;\
#X msg 27 146 0;\
#X obj 568 117 metro 130;\
#X obj 56 356 osc~;\
#X obj 431 121 random 60;\
canvas 121 146 915 638 adsr 0;\
#X obj 129 120 inlet;\
#X obj 438 160 inlet;\
#X obj 129 148 sel 0;\
#X obj 190 273 f \\$1;\
#X obj 495 160 inlet;\
#X obj 422 285 del \\$2;\
#X obj 595 456 line~;\
#X obj 446 313 f \\$4;\
#X obj 545 160 inlet;\
#X obj 600 160 inlet;\
#X obj 656 160 inlet;\
#X msg 129 179 stop;\
#X obj 596 315 pack 0 \\$5;\
#X obj 485 364 * \\$1;\
#X obj 595 486 outlet~;\
#X obj 446 338 * 0.01;\
#X obj 231 101 moses;\
#X obj 218 131 t b b;\
#X msg 152 299 0;\
#X obj 263 164 b;\
#X obj 190 298 pack f \\$2;\
#X obj 485 388 pack f \\$3;\
#X connect 0 0 2 0;\
#X connect 1 0 3 1;\
#X connect 1 0 13 1;\
#X connect 2 0 11 0;\
#X connect 2 0 12 0;\
#X connect 2 1 16 0;\
#X connect 3 0 20 0;\
#X connect 4 0 5 1;\
#X connect 4 0 20 1;\
#X connect 5 0 7 0;\
#X connect 6 0 14 0;\
#X connect 7 0 15 0;\
#X connect 8 0 21 1;\
#X connect 9 0 7 1;\
#X connect 10 0 12 1;\
#X connect 11 0 5 0;\
#X connect 12 0 6 0;\
#X connect 13 0 21 0;\
#X connect 15 0 13 0;\
#X connect 16 0 17 0;\
#X connect 16 1 19 0;\
#X connect 17 0 19 0;\
#X connect 17 1 18 0;\
#X connect 18 0 6 0;\
#X connect 19 0 3 0;\
#X connect 19 0 5 0;\
#X connect 20 0 6 0;\
#X connect 21 0 6 0;\
#X restore 30 255 pd adsr 10 5 50 50;\
#X f 30;\
#X obj 162 396 hsl 444 19 0 0.3 0 0 empty empty empty -2 -10 0 12 #fcfcfc #000000 #000000 0 1;\
#X obj 284 147 loadbang;\
#X obj 61 42 notein;\
#X obj 59 69 stripnote;\
#X obj 169 343 hsl 444 19 50 350 0 0 empty empty empty -2 -10 0 12 #fcfcfc #000000 #000000 0 1;\
#X msg 132 167 1;\
#X obj 467 215 + 3;\
#X obj 145 88 sel 0;\
#X obj 46 499 dac~;\
#X msg 367 224 1;\
#X obj 409 307 line;\
#X obj 520 218 + 69;\
#X obj 411 263 pack 0 633;\
#X msg 209 167 96;\
#X msg 269 212 \\; pd dsp 1;\
#X obj 510 512 line;\
#X obj 564 271 / 120;\
#X obj 512 468 pack 0 1233;\
#X connect 0 0 16 1;\
#X connect 1 0 17 0;\
#X connect 2 0 19 0;\
#X connect 3 0 6 0;\
#X connect 4 0 5 0;\
#X connect 5 0 28 0;\
#X connect 5 0 28 1;\
#X connect 6 0 4 0;\
#X connect 7 0 19 1;\
#X connect 8 0 19 4;\
#X connect 9 0 19 2;\
#X connect 10 0 19 3;\
#X connect 10 0 19 5;\
#X connect 11 0 3 0;\
#X connect 12 0 14 0;\
#X connect 12 0 16 0;\
#X connect 13 0 26 1;\
#X connect 14 0 15 0;\
#X connect 15 0 19 0;\
#X connect 16 0 2 0;\
#X connect 16 0 18 0;\
#X connect 17 0 3 1;\
#X connect 18 0 26 0;\
#X connect 18 0 31 0;\
#X connect 18 0 36 0;\
#X connect 19 0 11 0;\
#X connect 20 0 6 1;\
#X connect 21 0 33 0;\
#X connect 21 0 25 0;\
#X connect 21 0 29 0;\
#X connect 21 0 34 0;\
#X connect 22 0 23 0;\
#X connect 22 1 23 1;\
#X connect 23 0 1 0;\
#X connect 23 1 27 0;\
#X connect 24 0 7 0;\
#X connect 25 0 9 0;\
#X connect 26 0 1 0;\
#X connect 27 0 15 0;\
#X connect 27 1 2 0;\
#X connect 29 0 12 0;\
#X connect 30 0 24 0;\
#X connect 31 0 32 0;\
#X connect 32 0 30 0;\
#X connect 32 0 30 2;\
#X connect 33 0 7 0;\
#X connect 33 0 10 0;\
#X connect 35 0 20 0;\
#X connect 36 0 37 0;\
#X connect 37 0 35 0;\
#X connect 37 0 35 2;\
  ";


	ring_buffer *_inputRingBuffer;  ///< input buffer
	ring_buffer *_outputRingBuffer; ///< output buffer

    int main(void)
    {
        // initialize the hardware
        picoadk_init();
        exception_set_exclusive_handler(HARDFAULT_EXCEPTION, HardFault_Handler);

        sleep_ms(2000);
        printf("memory: free heap %ld of %ld\n" ,getFreeHeap(), getTotalHeap());

        int srate = 44100;
        int blksize = libpd_blocksize();
        libpd_set_verbose(1);
        libpd_set_printhook(pdprint);
        libpd_set_noteonhook(pdnoteon);
        libpd_init();
        libpd_init_audio(1, 1, srate);
        printf("blocksize %d\n", blksize);

        printf("memory: free heap %ld of %ld\n" ,getFreeHeap(), getTotalHeap());

        _inputRingBuffer = rb_create(sizeof(float) * 8 * blksize);
        _outputRingBuffer = rb_create(sizeof(float) * 8 * blksize);

        printf("memory: free heap %ld of %ld\n" ,getFreeHeap(), getTotalHeap());

        printf("\n*** before creating initial patch.\n");
        t_binbuf *b = binbuf_new();
        glob_setfilename(0, gensym("main-patch"), gensym("."));
        binbuf_text(b, oscpatchfile, strlen(oscpatchfile));
        // binbuf_text(b, westcoastpatchfile, strlen(westcoastpatchfile));
        binbuf_eval(b, &pd_canvasmaker, 0, 0);

        printf("firing loadbang\n");
        canvas_loadbang((t_canvas *)s__X.s_thing);
        vmess(s__X.s_thing, gensym("pop"), "i", 0);
        glob_setfilename(0, &s_, &s_);
        binbuf_free(b);
        printf("after creating initial patch.\n\n");

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
        Dsp_process_init(ctx);
        Dsp_default_init(ctx);
        Dsp_default(ctx);

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

        dsp_start = to_us_since_boot(get_absolute_time());

        // convert 12-bit adc value to 16-bit signed int
        // todo: use vult external function to read adcs instead
        uint32_t cv0 = adc128_read(0) * 16;
        uint32_t cv1 = rev_log_scale(adc128_read(1)) * 16;
        uint32_t cv2 = adc128_read(2) * 16;
        uint32_t cv3 = adc128_read(3) * 16;

        t_float inbuf[64] = {0,};
        t_float outbuf[64] = {0, };

        if (_inputRingBuffer && _outputRingBuffer) {

    		uint32_t framesAvailable = (uint32_t)rb_available_to_read(_outputRingBuffer) / (sizeof(float) * 1);
            if (framesAvailable < 64 ){
                libpd_process_float(1, inbuf, outbuf);
                rb_write_to_buffer(_outputRingBuffer, 64 * sizeof(float), (char *)&outbuf);
            }

    		framesAvailable = (uint32_t)rb_available_to_read(_outputRingBuffer) / (sizeof(float) * 1);
            if (framesAvailable >= 64 ){
                rb_read_from_buffer(_outputRingBuffer, (char *)&outbuf, buffer->max_sample_count*sizeof(float));
            }

            // this needs to be put in a ring buffer somewhere, and the correct length read out of it.
        }

        // We are filling the buffer with 32-bit samples (2 channels)
        for (uint i = 0; i < buffer->max_sample_count; i++)
        {
            // smp should be the output of your processing code.
            // In case of the Vult Example, this is Dsp_process(ctx);
            // Dsp_process(ctx, cv0, cv1, cv2, cv3);
            // fix16_t left_out = Dsp_process_ret_0(ctx);
            // fix16_t right_out = Dsp_process_ret_1(ctx);
            // samples[i * 2 + 0] = fix16_to_int32(left_out);  // LEFT
            // samples[i * 2 + 1] = fix16_to_int32(right_out); // RIGHT

            fix16_t left_out = float_to_fix(outbuf[i] * 0.5f);

            samples[i * 2 + 0] = fix16_to_int32(left_out);  // LEFT
            samples[i * 2 + 1] = fix16_to_int32(left_out); // RIGHT
        }

        dsp_end = to_us_since_boot(get_absolute_time());

        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(ap, buffer);
        return;
    }

#ifdef __cplusplus
}
#endif
