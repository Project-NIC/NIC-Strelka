# NIC-Strelka — Povodok's firmware: an nRF Connect SDK project on Zephyr, built with sysbuild for
# the nRF54LM20-DK, MCUboot beside it (FIRMWARE.md, The platform):
#
#     west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild firmware/povodok
#
# Built against nRF Connect SDK v3.4.0 with Zephyr SDK 1.0.1, MCUboot beside it; not run on
# hardware: the kit decides it (../README.md).
cmake_minimum_required(VERSION 3.20.0)
# the shared Kconfig first, the unit's own after it, so that the unit's values win; a file given
# by -D<project>_EXTRA_CONF_FILE comes after both
set(CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/../shared/common.conf ${CMAKE_CURRENT_LIST_DIR}/prj.conf)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(strelka_povodok LANGUAGES C)

set(COMMON ${CMAKE_CURRENT_LIST_DIR}/../common)
set(SHARED ${CMAKE_CURRENT_LIST_DIR}/../shared)
target_sources(app PRIVATE
  src/main.c
  ${SHARED}/clock.c
  ${SHARED}/crypto_psa.c
  ${SHARED}/light.c
  ${SHARED}/magnet.c
  ${SHARED}/usb.c
  ${SHARED}/gauge.c
  ${SHARED}/store.c
  ${SHARED}/pmic.c
  ${SHARED}/power.c
  ${SHARED}/strelka_mgmt_zephyr.c
  ${SHARED}/smp_tcp.c
  ${SHARED}/live_udp.c
  ${SHARED}/phone_net.c
  ${SHARED}/ble_prov.c
  ${COMMON}/crc32.c
  ${COMMON}/frame.c
  ${COMMON}/povodok_link.c
  ${COMMON}/live_msg.c
  ${COMMON}/frag.c
  ${COMMON}/smp.c
  ${COMMON}/strelka_mgmt.c
  ${COMMON}/fetch.c
  ${COMMON}/quat.c
  ${COMMON}/loop.c
)
target_include_directories(app PRIVATE src ${SHARED} ${COMMON})
# the fuel gauge's cell model: the SDK sample's, until the cell is profiled (shared/gauge.h)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/samples/pmic/native/npm13xx_fuel_gauge/src)
