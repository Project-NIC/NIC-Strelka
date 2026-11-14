/* NIC-Strelka — the download as Povodok runs it (COMMANDS.md, 64/21; PRINCIPLE.md, The download):
 * the phone asks once for a session from the Belkas, and Povodok goes round those of its group, a
 * frame at a time, of the one furthest behind — SMP's file download, 8/0, from the byte each has
 * reached, so that every file comes on together and the phone writes one C3D as it goes — passing
 * every answer on to the phone as it comes, on the download's port, so that the phone writes the
 * set's one file as it goes and waits on nothing. Every Povodok joined to the phone is given the
 * same Belkas: those that answer it, on its channel, are its own (PRINCIPLE.md, The download), one
 * that has not answered after FETCH_TRIES requests went to another and is left to it, and one
 * that has answered stays until her file is out, her error, or FETCH_GONE requests unanswered
 * running. The requests go over ESB by smp_esb.h, FETCH_TOKEN and her number their token. Plain C
 * on zcbor, held by tests/test_fetch.c and tests/test_world.c. */
#ifndef STRELKA_FETCH_H
#define STRELKA_FETCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

#define FETCH_TRIES  3u                 /* never answered: with another Povodok, … */
#define FETCH_ELSEWHERE_PERIODS 100u    /* … and 2 s since the first went unanswered, a Belka
					 * coming over to ESB at her next state's period found */
#define FETCH_GONE   30u                /* answered once, then silent: gone */
#define FETCH_TOKEN  0x80000000u        /* | the Belka's number */
#define FETCH_NAME   "/SD:/S%05uB%02u.STRK"
#define FETCH_NAME_MAX 24u

enum fetch_end {
	FETCH_RUNNING,
	FETCH_WHOLE,                    /* her file out to its end */
	FETCH_REFUSED,                  /* an error in her answer: the file not there */
	FETCH_ELSEWHERE,                /* never answered here: with another Povodok */
	FETCH_GONE_END,                 /* answered, then FETCH_GONE requests unanswered */
};

struct fetch {
	bool on;
	uint16_t s;                     /* the session */
	unsigned n;
	uint8_t belka[STRELKA_BELKAS_MAX];
	uint32_t off[STRELKA_BELKAS_MAX];   /* the next byte wanted */
	uint32_t len[STRELKA_BELKAS_MAX];   /* her file's, once told */
	bool len_known[STRELKA_BELKAS_MAX];
	uint8_t tries[STRELKA_BELKAS_MAX];  /* unanswered running */
	bool here[STRELKA_BELKAS_MAX];      /* she has answered this Povodok */
	uint32_t first_miss[STRELKA_BELKAS_MAX];   /* the period her first unanswered went */
	uint8_t end[STRELKA_BELKAS_MAX];    /* enum fetch_end */
	unsigned at;                    /* whose turn among those as far behind */
	int last_failed;                /* the one let go last, passed over once; -1 none */
	uint8_t seq;
};

/* the session s from the Belkas given, each from its byte in from, NULL from 0 */
void fetch_begin(struct fetch *f, uint16_t s, const uint8_t *belkas, const uint32_t *from,
		 unsigned n);
/* the next request, for the Belka furthest behind not done — of those as far, the next in turn;
 * the one let go last passed over once, should another be left: its frame's length, her number
 * in *belka; 0 when every one is done */
size_t fetch_next(struct fetch *f, uint8_t *belka, uint8_t *frame, size_t cap);
/* her answer: 1 taken, its data from *off, *data_len bytes at *data in the frame; 0 not the one
 * awaited, dropped; -1 her error, she is done */
int fetch_answer(struct fetch *f, uint8_t belka, const uint8_t *frame, size_t len, uint32_t *off,
		 const uint8_t **data, size_t *data_len);
/* her request let go unanswered in period now: asked again in her next turn; done after
 * FETCH_TRIES and FETCH_ELSEWHERE_PERIODS if she never answered here, after FETCH_GONE if she did */
void fetch_let_go(struct fetch *f, uint8_t belka, uint32_t now);
bool fetch_done(const struct fetch *f);
/* she has answered this Povodok: its own, a packet ESB gave up sent again, not let go */
bool fetch_here(const struct fetch *f, uint8_t belka);
/* her end, enum fetch_end; FETCH_RUNNING for a Belka not in the download */
uint8_t fetch_end_of(const struct fetch *f, uint8_t belka);

#endif
