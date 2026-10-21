#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include "card.h"

LOG_MODULE_REGISTER(card, LOG_LEVEL_INF);

#define DISK        "SD"
#define MOUNT       "/SD:"
#define RING_BLOCKS 256                    /* 128 KB: a quarter of the RAM (calc/params.py) */
#define SYNC_MS     3000                   /* the file's length saved every 3 s */
#define WRITER_PRIO 10                     /* below the sensor's and the radio's threads */

static FATFS fat_fs;
static struct fs_mount_t mnt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = MOUNT,
};
static struct fs_file_t file;
static bool present, mounted, file_open, closing;
static uint8_t fault;
static uint32_t dropped;

static uint8_t ring[RING_BLOCKS][REC_BLOCK_BYTES] __aligned(4);
static volatile uint32_t head, tail;       /* head the next free, tail the next to write */
static K_SEM_DEFINE(blocks_ready, 0, RING_BLOCKS);
static K_SEM_DEFINE(ring_empty, 0, 1);

int card_mount(void)
{
	int rc;

	if (mounted) {
		return 0;
	}
	rc = fs_mount(&mnt);
	if (rc) {
		fault = FAULT_FS_UNREADABLE;
		LOG_WRN("the card's file system unreadable (%d)", rc);
		return fault;
	}
	mounted = true;
	fault = FAULT_NONE;
	LOG_INF("the card mounted, %u MB free", card_free_mb());
	return 0;
}

int card_unmount(void)
{
	if (!mounted || file_open) {
		return file_open ? -EBUSY : 0;
	}
	fs_unmount(&mnt);
	mounted = false;
	return 0;
}

bool card_mounted(void)
{
	return mounted;
}

int card_init(void)
{
	int rc = disk_access_init(DISK);

	if (rc) {
		fault = FAULT_NO_CARD;
		LOG_WRN("no card (%d)", rc);
		return fault;
	}
	present = true;
	return card_mount();
}

uint8_t card_fault(void)
{
	return fault;
}

bool card_present(void)
{
	return present;
}

static bool free_room(struct fs_statvfs *st)
{
	return mounted && fs_statvfs(MOUNT, st) == 0;
}

uint8_t card_free_pct(void)
{
	struct fs_statvfs st;

	if (!free_room(&st) || st.f_blocks == 0) {
		return 0;
	}
	return (uint8_t)((uint64_t)st.f_bfree * 100 / st.f_blocks);
}

uint32_t card_free_mb(void)
{
	struct fs_statvfs st;

	if (!free_room(&st)) {
		return 0;
	}
	return (uint32_t)((uint64_t)st.f_bfree * st.f_frsize / (1024 * 1024));
}

int card_open(uint16_t session, uint8_t belka)
{
	char path[40];
	int rc;

	if (!mounted || file_open) {
		return -1;
	}
	snprintf(path, sizeof(path), MOUNT "/S%05uB%02u.STRK", session, belka);
	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (rc) {
		fault = FAULT_WRITE_FAILED;
		return rc;
	}
	/* the file's room is to be taken in one run here (RECORD.md, The file): FatFs's f_expand,
	 * which Zephyr's file API does not reach — the kit's work */
	head = tail = 0;
	dropped = 0;
	closing = false;
	file_open = true;
	return 0;
}

void card_put(const uint8_t block[REC_BLOCK_BYTES])
{
	uint32_t next = (head + 1) % RING_BLOCKS;

	if (!file_open || closing) {
		return;
	}
	if (next == tail || fault == FAULT_CARD_FULL) {
		dropped++;                       /* the buffer full, the card busy too long; or
						  * the card itself full */
		return;
	}
	memcpy(ring[head], block, REC_BLOCK_BYTES);
	head = next;
	k_sem_give(&blocks_ready);
}

uint32_t card_dropped(void)
{
	return dropped;
}

void card_close(void)
{
	if (file_open) {
		closing = true;                  /* the writer closes it once the ring is out */
		k_sem_give(&blocks_ready);
	}
}

bool card_busy(void)
{
	return file_open;
}

static void writer(void *a, void *b, void *c)
{
	int64_t last_sync = k_uptime_get();

	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	for (;;) {
		k_sem_take(&blocks_ready, K_MSEC(SYNC_MS));
		while (file_open && tail != head) {
			ssize_t n = fs_write(&file, ring[tail], REC_BLOCK_BYTES);

			if (n != REC_BLOCK_BYTES) {
				/* FatFs gives a short write, not -ENOSPC, when the volume is full */
				fault = n >= 0 ? FAULT_CARD_FULL : FAULT_WRITE_FAILED;
				LOG_ERR("a write failed (%d)", (int)n);
			}
			tail = (tail + 1) % RING_BLOCKS;
		}
		if (tail == head) {
			k_sem_give(&ring_empty);
			if (closing && file_open) {
				fs_close(&file);
				file_open = false;
				closing = false;
				continue;
			}
		}
		if (file_open && k_uptime_get() - last_sync >= SYNC_MS) {
			fs_sync(&file);              /* a power cut loses 3 s at most */
			last_sync = k_uptime_get();
		}
	}
}

K_THREAD_DEFINE(card_writer, 2048, writer, NULL, NULL, NULL, WRITER_PRIO, 0, 0);
