# PicoADK CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

set(PICO_EXTRAS_PATH "../lib/pico-extras")
set(PICO_BOARD datanoisetv_rp2350_dsp)
set(PICO_PLATFORM "rp2350-arm-s")
set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel")
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-sdk")


# Define values for the placeholders
set(FLASH_LENGTH 4M)      # Example value for FLASH length
set(PSRAM_LENGTH 8M)      # Example value for PSRAM length
set(RAM_LENGTH 520K)        # Example value for RAM length
set(EEPROM_START 0x1FFF000) # Example EEPROM start address
set(FS_START 0x08080000)    # Example file system start address
set(FS_END 0x080C0000)      # Example file system end address


# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
include(FreeRTOS_Kernel_import.cmake)

set(PICO_BOARD datanoisetv_rp2040_dsp)

set(LIBPD_ROOT ${CMAKE_CURRENT_LIST_DIR}/ext/pd/libpd)
set(FREERTOS_POSIX_PORT_PATH ${CMAKE_CURRENT_LIST_DIR}/ext/freertos-posix/)
set(FREERTOS_POSIX_REPO_PATH ${CMAKE_CURRENT_LIST_DIR}/ext/freertos-posix/Lab-Project-FreeRTOS-POSIX)
set(TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/ext/freertos-posix/Lab-Project-FreeRTOS-POSIX)
 
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
    # ${LIBPD_ROOT}/pure-data/src/d_soundfile.c
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        "."

        ${TINYUSB_PATH}/

        ${LIBPD_ROOT}/pure-data/src
        ${LIBPD_ROOT}/libpd_wrapper
        ${LIBPD_ROOT}/libpd_wrapper/util
)

add_subdirectory(lib/audio)
add_subdirectory(lib/oled)

# Set the real flag based on the PICO_PLATFORM variable
if (DEFINED PICO_PLATFORM AND PICO_PLATFORM STREQUAL "rp2350-arm-s")
    set(REAL_TYPE_FLAG "float")
    message("Using float for real type")
else()
    set(REAL_TYPE_FLAG "fixed")
    message("Using fixed for real type")
endif()


# TODO: PSRAM Linker Map
#configure_file(
#    ${CMAKE_SOURCE_DIR}/memmap.ld.in  # Input template file
#    ${CMAKE_BINARY_DIR}/memmap.ld     # Output linker script file
#    @ONLY                             # Use only @var@ style placeholders
#)

if (NOT FREERTOS_KERNEL_PATH AND NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message("Skipping FreeRTOS examples as FREERTOS_KERNEL_PATH not defined")
    return()
endif()

add_executable(main
    # vultsrc/dsp.vult
    src/usb_descriptors.c
    src/main.cpp
    src/picoadk_hw.cpp
    src/midi_input_usb.cpp
    src/get_serial.c
    src/espd.c
    src/freertos_port.c
    src/m0FaultDispatch.c
    ${PDSRC}
    ${PTHREADS_SRC}
    # ${CMAKE_CURRENT_BINARY_DIR}/vult.cpp
    lib/FreeRTOS-Kernel/portable/MemMang/heap_3.c
)

pico_set_program_name(main "main")
pico_set_program_version(main "0.1")

pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 1)

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
        hardware_sync_spin_lock
        hardware_sync
        hardware_exception
        pico_unique_id
        pico_bootsel_via_double_reset
        pico_util_buffer
        pico_multicore
        pico_stdio_uart
        tinyusb_board
        tinyusb_device
	    Audio
        #Oled
        FreeRTOS-Kernel
)

target_compile_options(main PRIVATE
        "-D_POSIX_THREADS"

        # Also defined by espd
        "-DPD"
        "-DPD_INTERNAL"
        "-DPD_HEADLESS"
        "-DSTUPID_SORT"
        "-DSYMTABHASHSIZE=512"
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
        PICO_USE_SW_SPIN_LOCKS=1
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

# compile the following thins only if rp2040 is used
if (PICO_PLATFORM STREQUAL "rp2040")
pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)

pico_set_boot_stage2(main slower_boot2)
else()
message("Skipping slower_boot2 as PICO_PLATFORM is not rp2040")
endif()

pico_add_extra_outputs(main)
