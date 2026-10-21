/* NIC-Strelka — the card: FatFs in Zephyr over the SD stack on SPIM20, a session a file, the
 * blocks through a buffer of 128 KB in RAM and a writer of its own below the sensor and the
 * radio, so that neither waits for the card; a block the buffer cannot hold is dropped, never
 * written late (FIRMWARE.md, The record). Built against nRF Connect SDK v3.4.0, not run on
 * hardware: the kit decides it. */
#ifndef STRELKA_CARD_H
#define STRELKA_CARD_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

int card_init(void);                       /* the card found and mounted; the fault, or 0 */
int card_mount(void);                      /* the file system taken: the record's, not the host's */
int card_unmount(void);                    /* let go, for the host on the cable to have the disk */
bool card_mounted(void);
uint8_t card_fault(void);                  /* FAULT_NO_CARD, FAULT_FS_UNREADABLE, FAULT_CARD_FULL,
					    * FAULT_WRITE_FAILED or FAULT_NONE */
bool card_present(void);
uint8_t card_free_pct(void);
uint32_t card_free_mb(void);
int card_open(uint16_t session, uint8_t belka);   /* S00007B03.STRK, as the record names it */
void card_put(const uint8_t block[REC_BLOCK_BYTES]);  /* into the buffer; the writer takes it */
void card_close(void);                     /* the writer closes the file once every block is
					    * out; card_busy says when it has */
bool card_busy(void);                      /* a file open or closing */
uint32_t card_dropped(void);               /* blocks dropped since the file was opened */

#endif
