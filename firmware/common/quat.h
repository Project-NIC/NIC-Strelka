/* NIC-Strelka — a turn in 4 B, "the smallest three": the largest of the quaternion's four
 * components is left out and named by 2 bits, the other three go in 10 bits each, scaled so
 * that ±1/√2, the most they can be, is ±511 (PRINCIPLE.md, The inside link; calc/params.py,
 * QUATERNION_BYTES). q and −q are one turn: the one with the largest component positive is
 * sent. The error is under 0,2° a component. */
#ifndef STRELKA_QUAT_H
#define STRELKA_QUAT_H

#include <stdint.h>

uint32_t quat_pack(const float q[4]);      /* w, x, y, z, unit length */
void quat_unpack(uint32_t v, float q[4]);

#endif
