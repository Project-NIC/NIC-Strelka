/* NIC-Strelka — the inside link's rules as numbers, the pages' and the firmware's own
 * (PRINCIPLE.md, The inside link; FIRMWARE.md, The inside link). The timetable in microseconds
 * is timetable.h's; here is what counts in periods. */
#ifndef STRELKA_LINK_RULES_H
#define STRELKA_LINK_RULES_H

#include "protocol.h"

#define LINK_PERIODS_A_S     STRELKA_PERIODS_A_S
#define LINK_SPOKEN_PERIODS 500u    /* on ESB until 10 s with no packet (smp_esb.h) */
#define LINK_ESB_HOME_PERIODS 30000u   /* a Povodok back on the set's channel from its own: silent
                                        * till the first's SYNC, 10 minutes at most */
#define LINK_ESB_ANNOUNCE   2u      /* the state's periods whose SYNCs announce ESB */
#define LINK_SPLIT_DB       6       /* the Povodoks a Belka shares herself among on ESB: within
                                     * this of the strongest SYNC heard */
#define LINK_STATE_EVERY     50u    /* with the live view off, a LIVE with the state alone once a
				     * second, every Belka in its slot in the same period, the
				     * period's number a multiple of 50 (PRINCIPLE.md, The
				     * download) */
#define LINK_WARM_PERIODS    (600u * LINK_PERIODS_A_S)   /* a session set, 64/1: the Belka awake, its
				     * sensor running and warming, for ten minutes with no record,
				     * live view or calibration — the warm-up before the six faces */
#define LINK_STOP_SYNCS      5u     /* SYNCs heard saying the record does not run: it stops */
#define LINK_STOP_S_DEFAULT  10u    /* seconds without a SYNC that pause the record, a setting */
#define LINK_OFF_S_DEFAULT   600u   /* seconds without a SYNC after which a Belka not recording
				     * switches itself off, every Povodok gone; recording but
				     * paused, three times as long, its end written — a setting */
#define LINK_ALONE_PERIODS   2u     /* periods without a SYNC: "on its own clock" in the state, and
				     * the spare channel tried every other period — two, since a
				     * move is said in two periods' SYNCs */
#define LINK_PERIOD_WINDOW   2u            /* a SYNC's period this far from the own count is taken;
                                           * further, a replay or a stranger's, while the set's time
                                           * is known and not lost (PRINCIPLE.md, The SYNC's signature) */
#define LINK_LOST_PERIODS    LINK_PERIODS_A_S  /* a second without a SYNC: a Belka listens a period
				     * on each of the set's other channels, every other period
				     * on the set's own between — where another Povodok stands up
				     * as the first after five seconds — "looking for the set"
				     * in the state; a Povodok of a higher number takes over
				     * instead */
#define LINK_TAKEOVER_PERIODS LINK_PERIODS_A_S /* no lower number heard for a second: this Povodok
				     * is the first — at a start too, every Povodok listening a
				     * second before its first SYNC, the set's time taken from
				     * whoever sends it meanwhile */
#define LINK_TAKEOVER_REST_PERIODS (3u * LINK_PERIODS_A_S)   /* the set at rest, nothing recorded
				     * and the live view off, its SYNCs in the state's period
				     * alone: no lower number heard for three seconds */
#define LINK_REST_SPARE      2u     /* at rest: the state's SYNCs missed running before a Belka
				     * listens on the spare in the next state's period */
#define LINK_REST_LOST       4u     /* … before it tries the set's other channels, one a state's
				     * period, the set's own every other */
#define LINK_TAKEOVER_LOST_PERIODS (5u * LINK_PERIODS_A_S)   /* a Povodok that has heard a lower
				     * number and hears none: silent this long before it stands up
				     * as the first — a lower number hidden a moment, an animal in
				     * the way, is not a first gone; under the Belkas' ten seconds */
#define LINK_SEARCH_ROUND    SEARCH_PERIODS    /* the CALL's attempt rises each round; in the
				     * state's periods the set's SYNC goes in the CALL's place, so
				     * that the set's own units are not left without one */
#define LINK_ESB_SYNC_PERIODS 3000u /* the set on ESB: once a minute the first leaves ESB for two
				     * of the state's periods running and sends its SYNC, bit 14 in
				     * it, so that a unit back on the SYNCs, done or never asked,
				     * hears the set and does not switch itself off; two, since
				     * such a unit listens on the set's own channel every other */
#define LINK_ESB_SYNCS       2u
#define LINK_MEASURED_CHANNELS STRELKA_SET_CHANNELS   /* every unit measures the set's 39 in
				     * turn, the search's left out: an odd count, so that a unit
				     * telling every other measurement, or one in a hundred at
				     * rest, still tells every channel */
/* the first Povodok's move, the firmware's own rule: the spare is the channel with the least
 * noise anyone measured, neither the current nor the search's; the set moves when the margin at
 * any Povodok heard within a second has stood under LINK_MOVE_MARGIN steps of 2 dB for
 * LINK_MOVE_PERIODS periods and the spare is quieter than the set's channel by
 * LINK_MOVE_QUIETER_DB as measured — a margin low for the distance alone moves nothing; the
 * move is said in the two SYNCs after the decision */
#define LINK_MOVE_MARGIN     5u     /* under 10 dB */
#define LINK_MOVE_PERIODS    LINK_PERIODS_A_S
#define LINK_MOVE_QUIETER_DB 6
#define LINK_NOISE_UNKNOWN   INT8_MIN
#define LINK_NOISE_FLOOR     (-100) /* taken for a channel no one measured yet */

#endif
