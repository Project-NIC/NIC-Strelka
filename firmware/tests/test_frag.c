/* An SMP frame in fragments over ESB and back (COMMANDS.md, The frame over ESB): 2 048 B in 9
 * fragments of 243, a frame of one, an empty one; a replay, a tampered byte, a stranger's key and
 * a fragment out of order dropped; and tags written to the file named, for Python to hold
 * against its own AES-CCM. */
#include <stdio.h>
#include <string.h>
#include "frag.h"
#include "test.h"

/* Povodok 0 sending to the Belka 5: the sender's and the receiver's who in the nonce */
static struct frag_keys povodok = { { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 },
				    0x52E6B438u, WHO(WHO_POVODOK, 0), WHO(WHO_BELKA, 5) };
static uint8_t frame[SMP_FRAME_MAX], out[16][ESB_PAYLOAD_MAX];
static size_t out_len[16];
static struct frag_rx rx;

static int send_all(size_t len, uint32_t *count)
{
	struct frag_tx tx;
	int n = 0;

	frag_tx_begin(&tx, frame, len);
	while ((out_len[n] = frag_tx_next(&tx, &povodok, count, out[n])) > 0) {
		n++;
	}
	return n;
}

int main(int argc, char **argv)
{
	uint32_t count = 0;
	int n;
	FILE *tags;

	CHECK(argc == 2);
	for (size_t i = 0; i < sizeof(frame); i++) {
		frame[i] = (uint8_t)(i * 7 + 3);
	}
	frag_rx_begin(&rx, 0);

	/* 2 048 B: 9 fragments, the first, seven middle, the last; whole again at the end */
	n = send_all(SMP_FRAME_MAX, &count);
	CHECK(n == 9 && count == 9 && out_len[0] == ESB_PAYLOAD_MAX && out_len[8] == 9 + 104);
	CHECK(FRAG_KIND(out[0][0]) == FRAG_FIRST && FRAG_KIND(out[4][0]) == FRAG_MIDDLE &&
	      FRAG_KIND(out[8][0]) == FRAG_LAST && FRAG_INDEX(out[8][0]) == 8);
	for (int i = 0; i < 8; i++) {
		CHECK(frag_rx_take(&rx, &povodok, out[i], out_len[i]) == FRAG_NONE);
	}
	CHECK(frag_rx_take(&rx, &povodok, out[8], out_len[8]) == FRAG_FRAME);
	CHECK(rx.len == SMP_FRAME_MAX && memcmp(rx.frame, frame, SMP_FRAME_MAX) == 0);
	OK("2 048 B in 9 fragments and back");

	/* a replay of the last fragment: its count not above the last taken */
	CHECK(frag_rx_take(&rx, &povodok, out[8], out_len[8]) == FRAG_DROPPED);
	/* a whole frame of 100 B, then a byte tampered, then a stranger's key */
	n = send_all(100, &count);
	CHECK(n == 1 && FRAG_KIND(out[0][0]) == FRAG_WHOLE && out_len[0] == 109);
	out[0][50] ^= 1;
	CHECK(frag_rx_take(&rx, &povodok, out[0], out_len[0]) == FRAG_DROPPED);
	out[0][50] ^= 1;
	{
		struct frag_keys stranger = povodok;

		stranger.key[0] ^= 1;
		CHECK(frag_rx_take(&rx, &stranger, out[0], out_len[0]) == FRAG_DROPPED);
	}
	CHECK(frag_rx_take(&rx, &povodok, out[0], out_len[0]) == FRAG_FRAME && rx.len == 100);
	/* the same fragment taken by another Belka: its receiver differs, the MIC fails */
	{
		struct frag_keys other = povodok;
		struct frag_rx rx7;

		other.receiver = WHO(WHO_BELKA, 7);
		frag_rx_begin(&rx7, 0);
		CHECK(frag_rx_take(&rx7, &other, out[0], out_len[0]) == FRAG_DROPPED);
	}
	OK("a replay, a tampered byte, a stranger, another Belka's fragment dropped");

	/* after a start the receiver takes no count under its floor: the saved one plus the
	 * saving's step; the sender past it goes on */
	{
		struct frag_rx fresh;
		uint32_t c = 100;

		frag_rx_begin(&fresh, 2048);
		n = send_all(10, &c);
		CHECK(frag_rx_take(&fresh, &povodok, out[0], out_len[0]) == FRAG_DROPPED);
		c = 2048;
		n = send_all(10, &c);
		CHECK(frag_rx_take(&fresh, &povodok, out[0], out_len[0]) == FRAG_FRAME);
	}
	OK("the count's floor after a start");

	/* fragments out of order: the first, then the third */
	n = send_all(600, &count);
	CHECK(n == 3);
	CHECK(frag_rx_take(&rx, &povodok, out[0], out_len[0]) == FRAG_NONE);
	CHECK(frag_rx_take(&rx, &povodok, out[2], out_len[2]) == FRAG_DROPPED);
	CHECK(frag_rx_take(&rx, &povodok, out[1], out_len[1]) == FRAG_DROPPED);
	/* an empty frame goes as one whole of 9 B */
	n = send_all(0, &count);
	CHECK(n == 1 && out_len[0] == FRAG_HEAD_BYTES + FRAG_MIC_BYTES);
	CHECK(frag_rx_take(&rx, &povodok, out[0], out_len[0]) == FRAG_FRAME && rx.len == 0);
	/* too short to be one */
	CHECK(frag_rx_take(&rx, &povodok, out[0], 8) == FRAG_DROPPED);
	OK("out of order, empty, too short");

	/* the tags for Python: key, nonce, the data, the tag, hex, a line each */
	tags = fopen(argv[1], "w");
	CHECK(tags != NULL);
	for (int t = 0; t < 6; t++) {
		uint8_t key[KEY_BYTES], nonce[NONCE_BYTES], data[300], tag[TAG_BYTES];
		size_t len = (size_t[]){ 0, 1, 15, 16, 17, 300 }[t];

		for (int i = 0; i < 16; i++) {
			key[i] = (uint8_t)(i * 13 + t);
		}
		frag_nonce(nonce, 0x11223344u + t, (uint8_t)(0x80 | t), (uint8_t)t, 1000u * t,
			   (uint8_t)t);
		for (size_t i = 0; i < len; i++) {
			data[i] = (uint8_t)(i * 31 + t);
		}
		strelka_ccm_encrypt(key, nonce, data, len, NULL, 0, NULL, tag);
		for (int i = 0; i < 16; i++) {
			fprintf(tags, "%02x", key[i]);
		}
		fputc(' ', tags);
		for (int i = 0; i < 13; i++) {
			fprintf(tags, "%02x", nonce[i]);
		}
		fputc(' ', tags);
		for (size_t i = 0; i < len; i++) {
			fprintf(tags, "%02x", data[i]);
		}
		fputc(' ', tags);
		for (int i = 0; i < 4; i++) {
			fprintf(tags, "%02x", tag[i]);
		}
		fputc('\n', tags);
	}
	/* and one encryption, the join's key under the agreed key: 16 B, aad none */
	{
		uint8_t key[16] = { 9 }, nonce[13] = { 8 }, in[16] = { 7 }, enc[16], dec[16], tag[4];

		strelka_ccm_encrypt(key, nonce, NULL, 0, in, 16, enc, tag);
		CHECK(memcmp(enc, in, 16) != 0);
		CHECK(strelka_ccm_decrypt(key, nonce, NULL, 0, enc, 16, dec, tag) == 0 &&
		      memcmp(dec, in, 16) == 0);
		tag[0] ^= 1;
		CHECK(strelka_ccm_decrypt(key, nonce, NULL, 0, enc, 16, dec, tag) != 0);
		for (int i = 0; i < 16; i++) {
			fprintf(tags, "%02x", key[i]);
		}
		fputc(' ', tags);
		for (int i = 0; i < 13; i++) {
			fprintf(tags, "%02x", nonce[i]);
		}
		fprintf(tags, " enc:");
		for (int i = 0; i < 16; i++) {
			fprintf(tags, "%02x", in[i]);
		}
		fputc(' ', tags);
		for (int i = 0; i < 16; i++) {
			fprintf(tags, "%02x", enc[i]);
		}
		tag[0] ^= 1;
		for (int i = 0; i < 4; i++) {
			fprintf(tags, "%02x", tag[i]);
		}
		fputc('\n', tags);
	}
	fclose(tags);
	OK("tags written for Python");
	printf("FRAG OK\n");
	return 0;
}
