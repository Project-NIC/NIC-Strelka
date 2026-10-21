/* NIC-Strelka — the CRC-32 every block of the record ends in: IEEE's, zlib's crc32, reflected,
 * polynomial 0xEDB88320, begun and ended with 0xFFFFFFFF (RECORD.md, The file). */
#ifndef STRELKA_CRC32_H
#define STRELKA_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t strelka_crc32(const void *data, size_t len);                /* one run */
uint32_t strelka_crc32_more(uint32_t crc, const void *data, size_t len);  /* run on */

#endif
