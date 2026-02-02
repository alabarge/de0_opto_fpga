cmake_minimum_required(VERSION 3.14)

add_subdirectory(../de0_bsp de0_bsp)

include(../de0_bsp/toolchain.cmake)

project(de0_fw)

enable_language(ASM)
enable_language(C)
enable_language(CXX)

# Pre-Build Command
add_custom_target(prebuild ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/pre_build.bat
    COMMENT "Create Version Header build.h"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)

add_executable(de0_fw.elf)

add_dependencies(de0_fw.elf prebuild)

target_sources(de0_fw.elf
    PRIVATE
    core/main.c
    core/ci.c
    core/lib.c
    core/main.c
    core/post.c
    core/cm.c
    core/cli_lib.c
    core/cli.c
    driver/gpio.c
    driver/xlprint.c
    driver/stamp.c
    driver/opto.c
    driver/adc.c
    cp_srv/cp_hal.c
    cp_srv/cp_srv.c
    daq_srv/daq_hal.c
    daq_srv/daq_srv.c
)

target_include_directories(de0_fw.elf PUBLIC
    .
    share
    core
    driver
    cp_srv
    daq_srv
)

target_link_libraries(de0_fw.elf
    PRIVATE
        -T "${BspLinkerScript}" -nostdlib
        "${ExtraArchiveLibraries}" -Wl,-Map=de0_fw.map
        -Wl,--start-group "${BspLibraryName}" -lc -lstdc++ -lgcc -lm -Wl,--end-group -Wl,--no-warn-rwx-segments
)

# Create objdump from ELF.
set(objdump de0_fw.elf.objdump)
add_custom_command(
    OUTPUT "${objdump}"
    DEPENDS de0_fw.elf
    COMMAND "${ToolchainObjdump}" "${ToolchainObjdumpFlags}" de0_fw.elf >
            "${objdump}"
    COMMENT "Creating ${objdump}."
    VERBATIM
)
add_custom_target(create-objdump ALL DEPENDS "${objdump}")

# Report space free for stack + heap. Note that the file below is never created
# so the report is always output on build.
set(stack_report_file de0_fw.elf.stack_report)
add_custom_command(
    OUTPUT "${stack_report_file}"
    DEPENDS de0_fw.elf
    COMMAND niosv-stack-report -p "${ToolchainPrefix}" de0_fw.elf
    COMMENT "Reporting memory available for stack + heap in de0_fw.elf."
    VERBATIM
)
add_custom_target(niosv-stack-report ALL DEPENDS "${stack_report_file}")

# Generate HEX file(s) from de0_fw.elf using elf2hex tool.
# Note : If ECC Full is enabled, width of 39 is set for NiosV TCM. Otherwise, 32.
# To generate the app.srec file (the pre-stage for the .hex), which makes to include the (default) boot-copier:
# (Remember the Reset vector is set to ECPS memory - with base address 0x02000000, and the Reset offset is 0x00100000)
add_custom_command(
    OUTPUT "sdram.hex"
    DEPENDS de0_fw.elf
    COMMAND elf2hex de0_fw.elf -o sdram.hex -b 0x00000000 -w 32 -e 0x01FFFFFF -r 4
    COMMENT "Creating sdram.hex."
    VERBATIM
)
add_custom_target(create-hex ALL DEPENDS "sdram.hex")

# To generate the de0_fw.flash file (the pre-stage for the .hex), which makes to include the (default) boot-copier:
# (Remember the Reset vector is set to ECPS memory - with base address 0x02000000, and the Reset offset is 0x00100000)
add_custom_command(
    OUTPUT "de0_fw.hex" "de0_fw.flash"
    DEPENDS de0_fw.elf
    COMMAND elf2flash.exe --input de0_fw.elf --output de0_fw.flash --reset 0x02100000 --base 0x02000000 --end 0x027fffff --boot ${CMAKE_CURRENT_SOURCE_DIR}/niosv_m_bootloader.srec
    COMMAND riscv32-unknown-elf-objcopy --input-target srec --output-target ihex de0_fw.flash de0_fw.hex
    COMMENT "Creating de0_fw.hex & de0_fw.flash."
    VERBATIM
)
add_custom_target(create-flash ALL DEPENDS "de0_fw.hex" "de0_fw.flash")

# Post-Build Command
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/zipfs.zip"
    DEPENDS de0_fw.elf
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/post_build.bat
    COMMENT "Create ZIP Filesystem zipfs.zip"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)
add_custom_target(create-zipfs ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/zipfs.zip")

