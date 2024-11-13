# PicoADK CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

set(PICO_EXTRAS_PATH "../lib/pico-extras")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

set(PICO_BOARD datanoisetv_rp2040_dsp)

set(FREERTOS_SRC_DIRECTORY "${PICO_SDK_PATH}/lib/tinyusb/lib/FreeRTOS-Kernel")
set(LIBPD_ROOT ${CMAKE_CURRENT_LIST_DIR}/ext/pd/libpd)
set(FREERTOS_POSIX_PORT_PATH ${CMAKE_CURRENT_LIST_DIR}/ext/freertos-posix/)
set(FREERTOS_POSIX_REPO_PATH ${CMAKE_CURRENT_LIST_DIR}/ext/freertos-posix/Lab-Project-FreeRTOS-POSIX)
set(USB_MIDI_HOST_ROOT ${CMAKE_CURRENT_LIST_DIR}/ext/usb_midi_host)

# include(${FREERTOS_POSIX_PATH}/freertos-posix-import.cmake)

add_library(FreeRTOS STATIC
    ${FREERTOS_SRC_DIRECTORY}/event_groups.c
    ${FREERTOS_SRC_DIRECTORY}/list.c
    ${FREERTOS_SRC_DIRECTORY}/queue.c
    ${FREERTOS_SRC_DIRECTORY}/stream_buffer.c
    ${FREERTOS_SRC_DIRECTORY}/tasks.c
    ${FREERTOS_SRC_DIRECTORY}/timers.c
#     ${FREERTOS_SRC_DIRECTORY}/portable/MemMang/heap_3.c
     ${FREERTOS_SRC_DIRECTORY}/portable/MemMang/heap_3.c
    ${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0/port.c

    # pthread compatibility
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_clock.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_mqueue.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_semaphore.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_timer.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_barrier.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_cond.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_mutex.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_unistd.c
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_utils.c
)

target_include_directories(FreeRTOS PUBLIC
    ${FREERTOS_CFG_DIRECTORY}/
    ${FREERTOS_SRC_DIRECTORY}/include
    ${FREERTOS_SRC_DIRECTORY}/portable/GCC/ARM_CM0

    ${FREERTOS_POSIX_PORT_PATH}/port
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/include/portable
    ${FREERTOS_POSIX_REPO_PATH}/FreeRTOS-Plus-POSIX/include
    ${FREERTOS_POSIX_REPO_PATH}/include/portable
    ${FREERTOS_POSIX_REPO_PATH}/include/private
    ${FREERTOS_POSIX_REPO_PATH}/include
)


set (PDSRC
    # PD Code
    ${LIBPD_ROOT}/pure-data/src/d_arithmetic.c
    ${LIBPD_ROOT}/pure-data/src/d_array.c
    ${LIBPD_ROOT}/pure-data/src/d_ctl.c
    ${LIBPD_ROOT}/pure-data/src/d_dac.c
    ${LIBPD_ROOT}/pure-data/src/d_delay.c
    ${LIBPD_ROOT}/pure-data/src/d_fft.c
    ${LIBPD_ROOT}/pure-data/src/d_fft_fftsg.c
    ${LIBPD_ROOT}/pure-data/src/d_filter.c
    ${LIBPD_ROOT}/pure-data/src/d_global.c
    ${LIBPD_ROOT}/pure-data/src/d_math.c
    ${LIBPD_ROOT}/pure-data/src/d_misc.c
    ${LIBPD_ROOT}/pure-data/src/d_osc.c
    ${LIBPD_ROOT}/pure-data/src/d_resample.c
    ${LIBPD_ROOT}/pure-data/src/d_soundfile_aiff.c
    ${LIBPD_ROOT}/pure-data/src/d_soundfile_caf.c
    ${LIBPD_ROOT}/pure-data/src/d_soundfile_next.c
    ${LIBPD_ROOT}/pure-data/src/d_soundfile_wave.c
    ${LIBPD_ROOT}/pure-data/src/d_soundfile.c
    ${LIBPD_ROOT}/pure-data/src/d_ugen.c

    ${LIBPD_ROOT}/pure-data/src/g_all_guis.c
    ${LIBPD_ROOT}/pure-data/src/g_array.c
        ${LIBPD_ROOT}/pure-data/src/g_bang.c
    ${LIBPD_ROOT}/pure-data/src/g_canvas.c
    ${LIBPD_ROOT}/pure-data/src/g_clone.c
        # ${LIBPD_ROOT}/pure-data/src/g_editor_extras.c
        # ${LIBPD_ROOT}/pure-data/src/g_editor.c
    ${LIBPD_ROOT}/pure-data/src/g_graph.c
    ${LIBPD_ROOT}/pure-data/src/g_guiconnect.c
    ${LIBPD_ROOT}/pure-data/src/g_io.c
        ${LIBPD_ROOT}/pure-data/src/g_mycanvas.c
        ${LIBPD_ROOT}/pure-data/src/g_numbox.c
        ${LIBPD_ROOT}/pure-data/src/g_radio.c
    ${LIBPD_ROOT}/pure-data/src/g_readwrite.c
    ${LIBPD_ROOT}/pure-data/src/g_rtext.c
    ${LIBPD_ROOT}/pure-data/src/g_scalar.c
        ${LIBPD_ROOT}/pure-data/src/g_slider.c
    ${LIBPD_ROOT}/pure-data/src/g_template.c
    ${LIBPD_ROOT}/pure-data/src/g_text.c
        ${LIBPD_ROOT}/pure-data/src/g_toggle.c
        ${LIBPD_ROOT}/pure-data/src/g_traversal.c
    # ${LIBPD_ROOT}/pure-data/src/g_undo.c
    # ${LIBPD_ROOT}/pure-data/src/g_vumeter.c

    ${LIBPD_ROOT}/pure-data/src/m_atom.c
    ${LIBPD_ROOT}/pure-data/src/m_binbuf.c
    ${LIBPD_ROOT}/pure-data/src/m_class.c
    # ${LIBPD_ROOT}/pure-data/src/m_conf.c
    # ${LIBPD_ROOT}/pure-data/src/m_glob.c
    ${LIBPD_ROOT}/pure-data/src/m_memory.c
    ${LIBPD_ROOT}/pure-data/src/m_obj.c
    ${LIBPD_ROOT}/pure-data/src/m_pd.c
    # ${LIBPD_ROOT}/pure-data/src/m_sched.c

    ${LIBPD_ROOT}/pure-data/src/s_audio.c
    ${LIBPD_ROOT}/pure-data/src/s_audio_dummy.c
    ${LIBPD_ROOT}/pure-data/src/s_inter_gui.c
    # ${LIBPD_ROOT}/pure-data/src/s_inter.c
    # ${LIBPD_ROOT}/pure-data/src/s_loader.c
    # ${LIBPD_ROOT}/pure-data/src/s_main.c
    # ${LIBPD_ROOT}/pure-data/src/s_net.c
    # ${LIBPD_ROOT}/pure-data/src/s_path.c
    ${LIBPD_ROOT}/pure-data/src/s_print.c
    ${LIBPD_ROOT}/pure-data/src/s_utf8.c

    ${LIBPD_ROOT}/pure-data/src/x_acoustics.c
    ${LIBPD_ROOT}/pure-data/src/x_arithmetic.c
    ${LIBPD_ROOT}/pure-data/src/x_array.c
    ${LIBPD_ROOT}/pure-data/src/x_connective.c
    # ${LIBPD_ROOT}/pure-data/src/x_file.c
    ${LIBPD_ROOT}/pure-data/src/x_gui.c
    ${LIBPD_ROOT}/pure-data/src/x_interface.c
    ${LIBPD_ROOT}/pure-data/src/x_list.c
    # ${LIBPD_ROOT}/pure-data/src/x_midi.c
    ${LIBPD_ROOT}/pure-data/src/x_misc.c
    # ${LIBPD_ROOT}/pure-data/src/x_net.c
    ${LIBPD_ROOT}/pure-data/src/x_scalar.c
    ${LIBPD_ROOT}/pure-data/src/x_text.c
    ${LIBPD_ROOT}/pure-data/src/x_time.c
    ${LIBPD_ROOT}/pure-data/src/x_vexp_fun.c
    ${LIBPD_ROOT}/pure-data/src/x_vexp_if.c
    ${LIBPD_ROOT}/pure-data/src/x_vexp.c

    # Some extras (if LIBPD_EXTRA is defined)
    ${LIBPD_ROOT}/pure-data/extra/bob~/bob~.c
    ${LIBPD_ROOT}/pure-data/extra/bonk~/bonk~.c
    ${LIBPD_ROOT}/pure-data/extra/choice/choice.c
    ${LIBPD_ROOT}/pure-data/extra/fiddle~/fiddle~.c
    ${LIBPD_ROOT}/pure-data/extra/loop~/loop~.c
    ${LIBPD_ROOT}/pure-data/extra/lrshift~/lrshift~.c
    # ${LIBPD_ROOT}/pure-data/extra/pd~/pd~.c
    # ${LIBPD_ROOT}/pure-data/extra/pd~/pdsched.c
    ${LIBPD_ROOT}/pure-data/extra/pique/pique.c
    ${LIBPD_ROOT}/pure-data/extra/sigmund~/sigmund~.c
    ${LIBPD_ROOT}/pure-data/extra/stdout/stdout.c

    # LibPD Wrapper
    ${LIBPD_ROOT}/libpd_wrapper/z_hooks.c
    ${LIBPD_ROOT}/libpd_wrapper/z_libpd.c
    ${LIBPD_ROOT}/libpd_wrapper/x_libpdreceive.c
    ${LIBPD_ROOT}/libpd_wrapper/util/z_print_util.c
    ${LIBPD_ROOT}/libpd_wrapper/s_libpdmidi.c
    ${LIBPD_ROOT}/libpd_wrapper/util/z_queued.c
    ${LIBPD_ROOT}/libpd_wrapper/util/z_ringbuffer.c
)

project(main C CXX ASM)

pico_sdk_init()

include_directories(
        ${USB_MIDI_HOST_ROOT}/

        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        "."

        ${LIBPD_ROOT}/pure-data/src
        ${LIBPD_ROOT}/libpd_wrapper
        ${LIBPD_ROOT}/libpd_wrapper/util
)

add_subdirectory(lib/audio)
add_subdirectory(lib/oled)

message("CMAKE_HOST_SYSTEM_PROCESSOR: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message("CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME}")

# Generate Vult C++ code from Vult Code every time
set(VULT_EXAMPLE_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/vult/examples")
add_custom_command(
        OUTPUT ALL ${CMAKE_CURRENT_BINARY_DIR}/vult.cpp
        COMMAND vultc ${CMAKE_CURRENT_LIST_DIR}/vultsrc/dsp.vult
                -i ${VULT_EXAMPLE_PATH}/osc
                -i ${VULT_EXAMPLE_PATH}/util
                -i ${VULT_EXAMPLE_PATH}/filters
                -i ${VULT_EXAMPLE_PATH}/env
                -i ${VULT_EXAMPLE_PATH}/midi
                -i ${VULT_EXAMPLE_PATH}/effects
                -i ${VULT_EXAMPLE_PATH}/units
                -ccode
                -real fixed
                -samplerate 44140
                -o vult
        COMMENT "Transcompiling Vult DSP Code to C++."
)

add_executable(main
    vultsrc/dsp.vult
    src/main.cpp
    src/picoadk_hw.cpp
    src/vultin.cpp
    src/get_serial.c
    src/espd.c
    src/freertos_port.c
    src/m0FaultDispatch.c
    ${PDSRC}
    ${CMAKE_CURRENT_BINARY_DIR}/vult.cpp
    ${USB_MIDI_HOST_ROOT}/usb_midi_host_app_driver.c
    ${USB_MIDI_HOST_ROOT}/usb_midi_host.c
)

pico_set_program_name(main "main")
pico_set_program_version(main "0.1")

pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 0)

# Add the standard library to the build
target_link_libraries(main pico_stdlib)

# Add any user requested libraries
target_link_libraries(main
        hardware_adc
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        hardware_exception
        pico_unique_id
        pico_bootsel_via_double_reset
        pico_util_buffer
        pico_multicore
        pico_stdio_uart
        # pico_stdio_usb
        # tinyusb_device
        tinyusb_host
        tinyusb_board
	    Audio
        FreeRTOS
)

target_compile_options(main PRIVATE
        "-D_POSIX_THREADS"

        # Also defined by espd
        "-DPD"
        "-DPD_INTERNAL"
        "-DPD_HEADLESS"
        "-DSTUPID_SORT"
        # "-DSYMTABHASHSIZE=512"
        "-DHAVE_UNISTD_H"
        "-DHAVE_ALLOCA_H"

        # PD Extra stuff
        "-UPDINSTANCE"
        "-DUSEAPI_DUMMY"
        # "-DLOUD"
        "-DDEBUGMEM"
        "-DLIBPD_EXTRA"

        # My hacky stuff
        "-DPICO=${PICOFLAG}"
        "-DNONET"

        "-DPDTHREADS=0"
        "-DLWIP"
        "-DLWIP_COMPAT_SOCKETS=2"
        "-DHAVE_MACHINE_ENDIAN_H"
)

target_compile_definitions(main PRIVATE
        USE_AUDIO_I2S=1
        USE_USB_MIDI=1
)

target_link_options(main PRIVATE
        -Xlinker --print-memory-usage
)

file(GLOB files "${CMAKE_CURRENT_LIST_DIR}/piosrc/*.pio")
foreach(file ${files})
    message ("Adding ${file} to PIO build.")
    pico_generate_pio_header(main ${file})
endforeach()

add_compile_options("-Wall" "-Wredundant-decls")

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)

pico_set_boot_stage2(main slower_boot2)

pico_add_extra_outputs(main)
