/* NIC-Strelka — the period's timetable in microseconds from its start, computed from the packets'
 * sizes and the radio's figures as calc/radio.py computes them for PRINCIPLE.md (The inside
 * link): the SYNCs first, the Belkas' slots, the MISS, the PASSes, the measurement at the end;
 * and the search's windows after a CALL. The host test holds the figures to the page's. */
#ifndef STRELKA_TIMETABLE_H
#define STRELKA_TIMETABLE_H

#include <stdint.h>
#include "protocol.h"

#define TT_GUARD_US        30u     /* before the first SYNC */
#define TT_TURN_US         50u     /* from receiving to sending: the receiver off in 1 µs, the
				    * fast ramp-up 40 µs, the rest a margin (the datasheet,
				    * 11.14.8) */
#define TT_SLOT_GAP_US     36u     /* between two Belkas' packets */
#define TT_MEASURE_US      200u    /* a Povodok's turn to another channel at the period's end */

#define TT_SYNC_AIR_US     STRELKA_AIR_US(SYNC_BODY_BYTES)              /* 88 */
#define TT_SYNC_SLOT_US    (TT_SYNC_AIR_US + TT_TURN_US)                /* 138 */
#define TT_SYNC_AT_US(k)   (TT_GUARD_US + (k) * TT_SYNC_SLOT_US)        /* Povodok k's SYNC */
#define TT_SLOTS_AT_US     TT_SYNC_AT_US(STRELKA_POVODOKS_MAX)          /* 582 */
#define TT_SLOT_US         (STRELKA_AIR_US(LIVE_BODY_MAX) + TT_SLOT_GAP_US)   /* 204 */
#define TT_SLOT_AT_US(n)   (TT_SLOTS_AT_US + ((n) - 1u) * TT_SLOT_US)   /* the Belka n's, 1–63 */
#define TT_MISS_AT_US      TT_SLOT_AT_US(STRELKA_BELKAS_MAX + 1u)       /* 13 434 */
#define TT_MISS_US         (STRELKA_AIR_US(MISS_BODY_BYTES) + TT_TURN_US)     /* 154 */
#define TT_PASSES_AT_US    (TT_MISS_AT_US + TT_MISS_US)                 /* 13 588 */
#define TT_MEASURE_AT_US   (STRELKA_PERIOD_US - TT_MEASURE_US)          /* 19 800 */
#define TT_PASS_WINDOW_US  ((TT_MEASURE_AT_US - TT_PASSES_AT_US) / (STRELKA_POVODOKS_MAX - 1u))
#define TT_PASS_AT_US(j)   (TT_PASSES_AT_US + ((j) - 1u) * TT_PASS_WINDOW_US) /* Povodok j, 1–3 */
/* a HERE's window w, 0–137, counted from the CALL's end */
#define TT_HERE_AT_US(w)   (TT_TURN_US + (w) * SEARCH_WINDOW_US)

_Static_assert(TT_SYNC_AIR_US == 88 && TT_SYNC_SLOT_US == 138, "a SYNC, PRINCIPLE.md");
_Static_assert(TT_SLOTS_AT_US == 582 && TT_SLOT_US == 204, "the slots, PRINCIPLE.md");
_Static_assert(TT_MISS_AT_US == 13434 && TT_MISS_US == 154, "the MISS, PRINCIPLE.md");
_Static_assert(TT_PASS_WINDOW_US >= 2070 && TT_PASS_WINDOW_US < 2080, "a PASS window, 2,07 ms");
_Static_assert(TT_HERE_AT_US(SEARCH_WINDOWS - 1u) + STRELKA_AIR_US(HERE_BODY_BYTES) +
	       TT_SYNC_AIR_US < STRELKA_PERIOD_US, "the last HERE ends before the next CALL");

#endif
