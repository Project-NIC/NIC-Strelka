# NIC-Strelka — a Zephyr module built into MCUboot alone: the VBUS gate.
if(CONFIG_STRELKA_VBUS_GATE)
  zephyr_library()
  zephyr_library_sources(vbusgate.c)
endif()
