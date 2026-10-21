/* NIC-Strelka — the inside link's rules as numbers, the pages' and the firmware's own
 * (PRINCIPLE.md, The inside link; FIRMWARE.md, The inside link). The timetable in microseconds
 * is timetable.h's; here is what counts in periods. */
#ifndef STRELKA_LINK_RULES_H
#define STRELKA_LINK_RULES_H

#include "protocol.h"

#define LINK_PERIODS_A_S     STRELKA_PERIODS_A_S
#define LINK_STATE_EVERY     50u    /* with the live view off, a LIVE with the state alone once a
				     * second, every Belka in its slot in the same period, the
				     * period's number a multiple of 50 (PRINCIPLE.md, The
				     * download) */
#define LINK_ITEMS_EVERY     (60u * LINK_PERIODS_A_S)  /* once a minute the SYNC asks items 1–7 in
				     * seven periods in a row — seven of the periods the Belkas
				     * send in, with the live view off one a second */
#define LINK_STOP_SYNCS      5u     /* SYNCs heard saying the record does not run: it stops */
#define LINK_STOP_S_DEFAULT  10u    /* seconds without a SYNC that pause the record, a setting */
#define LINK_ALONE_PERIODS   2u     /* periods without a SYNC: "on its own clock" in the state, and
				     * the spare channel tried every other period — two, since a
				     * move is said in two periods' SYNCs */
#define LINK_LOST_PERIODS    LINK_PERIODS_A_S  /* a second without a SYNC: a Belka listens a period
				     * on each of the set's other channels, every other period
				     * on the set's own between — where another Povodok stands up
				     * as the first after the same second — "looking for the set"
				     * in the state; a Povodok of a higher number takes over
				     * instead */
#define LINK_TAKEOVER_PERIODS LINK_PERIODS_A_S /* no lower number heard for a second: this Povodok
				     * is the first — at a start too, every Povodok listening a
				     * second before its first SYNC, the set's time taken from
				     * whoever sends it meanwhile */
#define LINK_SEARCH_ROUND    SEARCH_PERIODS    /* the CALL's attempt rises each round */
/* the first Povodok's move, the firmware's own rule: the spare is the channel with the least
 * noise anyone measured, neither the current nor the search's; the set moves when the margin at
 * any Povodok heard within a second has stood under LINK_MOVE_MARGIN steps of 2 dB for
 * LINK_MOVE_PERIODS periods; the move is said in the two SYNCs after the decision */
#define LINK_MOVE_MARGIN     5u     /* under 10 dB */
#define LINK_MOVE_PERIODS    LINK_PERIODS_A_S
#define LINK_NOISE_UNKNOWN   INT8_MIN
#define LINK_NOISE_FLOOR     (-100) /* taken for a channel no one measured yet */

#endif
