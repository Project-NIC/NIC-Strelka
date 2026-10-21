# NIC-Strelka — a Zephyr module built into MCUboot alone: the hold and the VBUS gate.
if(CONFIG_STRELKA_VBUS_GATE OR (DEFINED CONFIG_STRELKA_HOLD_PIN AND NOT CONFIG_STRELKA_HOLD_PIN EQUAL -1))
  zephyr_library()
  zephyr_library_sources(vbusgate.c)
endif()
