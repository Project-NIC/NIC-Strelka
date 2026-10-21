/* NIC-Strelka — the sensor's clock steered to the set's grid, the hardware's side (FIRMWARE.md,
 * The sensor and its clock). CLKIN, 32 kHz, is TIMER21 toggling the sensor's pin 9 through
 * GPIOTE20: 250 ticks of the 16 MHz clock a half period. TIMER22 runs free at 16 MHz and
 * captures the data-ready and the SYNC's END in hardware. The loop's arithmetic is
 * common/loop.c's, run on the host against a simulated crystal; this file is the timers, the
 * pins and the stamps, built against nRF Connect SDK v3.4.0, not run on hardware: the kit decides it. */
#ifndef STRELKA_CLOCK_H
#define STRELKA_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

#define CLOCK_HZ            16000000u
#define CLOCK_TICK_NS       62.5
#define CLOCK_PERIOD_TICKS  (20000u * 16u)

int clock_init(void);
/* from the GPIOTE interrupt at every INT1 edge, with the edge's stamp captured in hardware */
void clock_on_data_ready(void (*cb)(uint64_t stamp));
void clock_data_ready_irq(bool on);          /* that interrupt on or off; the capture stays */
void clock_clkin_start(bool steered);        /* steered, or the fallback: 500 ticks, no steps */
void clock_clkin_stop(void);

/* the stamps, in ticks of the free timer, extended to 64 bits */
uint64_t clock_now(void);
uint64_t clock_last_data_ready(void);        /* the last INT1 edge, captured in hardware */
uint64_t clock_last_sync_end(void);          /* the last SYNC's END, captured in hardware */

/* once a period: the SYNC heard, its END on the local timer, its period's number and the
 * Povodok that sent it, so that the period's start is known; or heard = false */
void clock_sync(bool heard, uint64_t end_local, uint32_t period, uint8_t povodok);
void clock_set_sample_period(uint32_t ticks);     /* the sensor's rate: 16e6 / rate_hz */
bool clock_locked(void);                          /* within a microsecond for a second */
bool clock_origin_known(void);                    /* a SYNC heard since the start */
uint64_t clock_period_start(uint32_t period);     /* on the local timer */
int32_t clock_grid_offset_ticks(uint64_t stamp);  /* where a data-ready fell off the grid */
int64_t clock_sample_index(uint64_t stamp);       /* the sample's number on the grid, from the
						   * set's period 0; -1 before the first SYNC */
uint32_t clock_samples_a_period(void);

#endif
