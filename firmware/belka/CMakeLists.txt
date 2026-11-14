# NIC-Strelka — the Belka's firmware: an nRF Connect SDK project on Zephyr, built with sysbuild
# for the nRF54LM20-DK, MCUboot beside it (FIRMWARE.md, The platform):
#
#     west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild firmware/belka
#
# Built against nRF Connect SDK v3.4.0 with Zephyr SDK 1.0.1, MCUboot beside it; not run on
# hardware: the kit decides it (../README.md).
cmake_minimum_required(VERSION 3.20.0)
# the shared Kconfig first, the unit's own after it, so that the unit's values win; a file given
# by -D<project>_EXTRA_CONF_FILE comes after both
set(CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/../shared/common.conf ${CMAKE_CURRENT_LIST_DIR}/prj.conf)
# Kryska: the Belka's firmware for Kryska's board, its devicetree and configuration laid over, by
# sysbuild's own variables for the image (boards/kryska.overlay):
#
#     west build -b nrf54lm20dk/nrf54lm20a/cpuapp --sysbuild firmware/belka --
#         -Dbelka_EXTRA_CONF_FILE=kryska.conf -Dbelka_EXTRA_DTC_OVERLAY_FILE=boards/kryska.overlay
#
# its other way, the NOR flash, by kryska_nor.conf and boards/kryska_nor.overlay the same
# the sensor on the FLPR; first, so that the fallback's overlay, given after it, wins
# (boards/sensor_on_app.overlay, with CONFIG_STRELKA_SENSOR_FLPR=n)
list(PREPEND EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/boards/flpr.overlay)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(strelka_belka LANGUAGES C)

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
  ${COMMON}/cal.c
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
  ${COMMON}/imu_tables.c
  ${COMMON}/resample.c
  ${COMMON}/setfile.c
)
# the Belka's card, Kryska's SD NAND too, or on Kryska's other way its NOR flash behind the card's
# interface, the record raw on it (chapters/kryska-memory.md)
if(CONFIG_DISK_DRIVER_SDMMC)
  target_sources(app PRIVATE src/card.c)
else()
  target_sources(app PRIVATE src/flash_rec.c ${COMMON}/flashrec.c)
endif()
if(CONFIG_STRELKA_SENSOR_FLPR)
  target_sources(app PRIVATE src/sensor_flpr.c)
else()
  target_sources(app PRIVATE src/imu.c)
endif()
target_include_directories(app PRIVATE src ${SHARED} ${COMMON})
# the fuel gauge's cell model: the SDK sample's, until the cell is profiled (shared/gauge.h)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/samples/pmic/native/npm13xx_fuel_gauge/src)
