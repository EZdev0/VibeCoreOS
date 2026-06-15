/*
 * ============================================================
 *  VibeCore OS — Mailbox-Interface Header
 * ============================================================
 */

#ifndef _MAILBOX_H
#define _MAILBOX_H

#include "types.h"

/* Mailbox Property-Tag Buffer Layout
 *
 *  Offset  Size   Description
 *  ──────────────────────────────────────────────
 *   0       4      Buffer size (Bytes)
 *   4       4      Request/Response-Code (0 = Request)
 *   8       ...    Tags (aufeinanderfolgend)
 *   (Ende)  4      End-Tag (0x00000000)
 *
 *  Jeder Tag:
 *   0       4      Tag-ID
 *   4       4      Value buffer size (Bytes)
 *   8       4      Request/Response (Bit 31 = Request)
 *   12      N      Value-Buffer
 */


/* ── Mailbox Property IDs ─────────────────────────────────── */
#define MBOX_TAG_GET_FIRMWARE      0x00000001
#define MBOX_TAG_GET_BOARD_MODEL   0x00010001
#define MBOX_TAG_GET_BOARD_REV     0x00010002
#define MBOX_TAG_GET_MAC_ADDRESS   0x00010003
#define MBOX_TAG_GET_BOARD_SERIAL  0x00010004
#define MBOX_TAG_GET_ARM_MEMORY    0x00010005
#define MBOX_TAG_GET_VC_MEMORY     0x00010006
#define MBOX_TAG_SET_CLKRATE       0x00038002

/* ── Framebuffer-Tags ────────────────────────────────────── */
#define MBOX_TAG_FB_ALLOCATE           0x00040001
#define MBOX_TAG_FB_RELEASE            0x00048001
#define MBOX_TAG_FB_BLANK              0x00040002
#define MBOX_TAG_FB_GET_PHYSICAL_DIM   0x00040003
#define MBOX_TAG_FB_SET_PHYSICAL_DIM   0x00048003
#define MBOX_TAG_FB_GET_VIRTUAL_DIM    0x00040004
#define MBOX_TAG_FB_SET_VIRTUAL_DIM    0x00048004
#define MBOX_TAG_FB_GET_DEPTH          0x00040005
#define MBOX_TAG_FB_SET_DEPTH          0x00048005
#define MBOX_TAG_FB_GET_PIXEL_ORDER    0x00040006
#define MBOX_TAG_FB_SET_PIXEL_ORDER    0x00048006
#define MBOX_TAG_FB_GET_PITCH          0x00040008
#define MBOX_TAG_FB_GET_VIRTUAL_OFFSET 0x00040009
#define MBOX_TAG_FB_SET_VIRTUAL_OFFSET 0x00048009
#define MBOX_TAG_FB_GET_OVERSCAN       0x0004000A
#define MBOX_TAG_FB_SET_OVERSCAN       0x0004800A
#define MBOX_TAG_FB_GET_PALETTE        0x0004000B

/* ── Funktionen ──────────────────────────────────────────── */
bool mailbox_call(u32 *buffer, u8 channel);

#endif /* _MAILBOX_H */
