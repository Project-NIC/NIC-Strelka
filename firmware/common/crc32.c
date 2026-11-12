#include "crc32.h"

/* the table built once, 1 KB: the main core takes some 30 µs a block of 508 B with it */
static uint32_t table[256];
static int table_ready;

static void make_table(void)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t c = i;

		for (int k = 0; k < 8; k++) {
			c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
		}
		table[i] = c;
	}
	table_ready = 1;
}

uint32_t strelka_crc32_more(uint32_t crc, const void *data, size_t len)
{
	const uint8_t *p = data;

	if (!table_ready) {
		make_table();
	}
	crc = ~crc;
	while (len--) {
		crc = table[(crc ^ *p++) & 0xFFu] ^ (crc >> 8);
	}
	return ~crc;
}

uint32_t strelka_crc32(const void *data, size_t len)
{
	return strelka_crc32_more(0, data, len);
}
