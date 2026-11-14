# NIC-Strelka — the FLPR's program: the sensor alone, on the nRF54LM20A's RISC-V coprocessor
# (FIRMWARE.md, The sensor on the FLPR). Built apart from the Belka's sysbuild, as Nordic's guide
# asks (the SDK's nrf54l_signing_with_payload.rst), then merged into the Belka's signed image by
# flpr/merge.py:
#
#     west build -b nrf54lm20dk/nrf54lm20a/cpuflpr --no-sysbuild -d build-flpr firmware/flpr
#
# Built against nRF Connect SDK v3.4.0, not run on hardware: the kit decides it.
cmake_minimum_required(VERSION 3.20.0)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_LIST_DIR}/../belka)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(strelka_flpr LANGUAGES C)

set(COMMON ${CMAKE_CURRENT_LIST_DIR}/../common)
set(BELKA ${CMAKE_CURRENT_LIST_DIR}/../belka/src)
target_sources(app PRIVATE
  src/main.c
  ${BELKA}/imu.c
  ${COMMON}/imu_tables.c
)
target_include_directories(app PRIVATE ${BELKA} ${COMMON})
