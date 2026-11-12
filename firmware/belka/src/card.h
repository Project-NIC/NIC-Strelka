/* NIC-Strelka — the card: FatFs in Zephyr over the SD stack on SPIM20, a session a file, the
 * blocks through a buffer of 128 KB in RAM and a writer of its own below the sensor and the
 * radio, so that neither waits for the card; a block the buffer cannot hold is dropped, never
 * written late (FIRMWARE.md, The record). Kryska's SD NAND is this card; on Kryska's other way
 * the same interface stands before its NOR flash, flash_rec.c, the record raw on it
 * (chapters/kryska-memory.md). Built against nRF Connect SDK v3.4.0, not run
 * on hardware: the kit decides it. */
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
/* the highest rate the memory keeps with both of the hoof's accelerometer channels: the card any,
 * the NOR way's flash 2 kHz, its pages within the sheet's longest time at it
 * (chapters/kryska-memory.md) */
uint32_t card_both_most_hz(void);
uint8_t card_fs(void);                     /* 0 none, 1 FAT32, 2 exFAT (COMMANDS.md, info) */
uint8_t card_free_pct(void);
uint32_t card_free_mb(void);
/* S00007B03.STRK, as the record names it, its room taken in one run for a day at rate_hz, the
 * record's own rate; given back at the close */
int card_open(uint16_t session, uint8_t belka, uint32_t rate_hz);
void card_put(const uint8_t block[REC_BLOCK_BYTES]);  /* into the buffer; the writer takes it */
void card_close(void);                     /* the writer closes the file once every block is
					    * out; card_busy says when it has */
bool card_busy(void);                      /* a file open or closing */
uint32_t card_dropped(void);               /* blocks dropped since the file was opened */
/* the file being written, its name in the mount and the bytes in it so far, for 64/2: on the
 * card its size is the room taken at the start; false when none is open (Kryska's lists its
 * sessions as they stand) */
bool card_recording(char *name, size_t cap, uint64_t *bytes);
/* 64/5: 1 every session erased, 2 the card formatted — on the NOR way both the whole memory erased,
 * by its writer, card_busy saying while it runs; 0, or -EBUSY while a record runs, -ENODEV with
 * no memory, -EIO when it failed */
int card_erase(uint8_t what);
/* STRELKA.SET given by the cable (FIRMWARE.md, What a unit keeps): on Kryska over SMP on the
 * serial port into the record's file system, allowed where a Belka's disk would be open, and
 * waiting then for main.c to take it as from a disk; the card's own never has one, the file
 * being on its disk */
void card_set_file_allowed(bool allowed);
bool card_set_file_given(void);

#endif
