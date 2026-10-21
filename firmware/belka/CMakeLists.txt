# NIC-Strelka — the Belka's firmware: an nRF Connect SDK project on Zephyr, built with sysbuild
# for the nRF54LM20-DK, MCUboot beside it (FIRMWARE.md, The platform):
#
#     west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild firmware/belka
#
# Built against nRF Connect SDK v3.4.0 with Zephyr SDK 1.0.1, MCUboot beside it; not run on
# hardware: the kit decides it (../README.md).
cmake_minimum_required(VERSION 3.20.0)
set(EXTRA_CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/../shared/common.conf)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(strelka_belka LANGUAGES C)

set(COMMON ${CMAKE_CURRENT_LIST_DIR}/../common)
set(SHARED ${CMAKE_CURRENT_LIST_DIR}/../shared)
target_sources(app PRIVATE
  src/main.c
  src/imu.c
  src/card.c
  ${SHARED}/clock.c
  ${SHARED}/crypto_psa.c
  ${SHARED}/light.c
  ${SHARED}/hold.c
  ${SHARED}/magnet.c
  ${SHARED}/usb.c
  ${SHARED}/gauge.c
  ${SHARED}/charger.c
  ${SHARED}/strelka_mgmt_zephyr.c
  ${COMMON}/crc32.c
  ${COMMON}/record.c
  ${COMMON}/session.c
  ${COMMON}/steps.c
  ${COMMON}/frame.c
  ${COMMON}/belka_link.c
  ${COMMON}/orient.c
  ${COMMON}/frag.c
  ${COMMON}/smp.c
  ${COMMON}/strelka_mgmt.c
  ${COMMON}/quat.c
  ${COMMON}/loop.c
)
target_include_directories(app PRIVATE src ${SHARED} ${COMMON})
