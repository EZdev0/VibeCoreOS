/*
 * ============================================================
 *  VibeCore OS — Mailbox Interface
 *
 *  Communication between ARM CPU and VideoCore GPU.
 *  Used for framebuffer initialization and
 *  hardware configuration.
 *
 *  Protocol:
 *    1. Fill buffer (16-byte-aligned) with property tags
 *    2. Write buffer address to mailbox channel 8
 *    3. Wait for response (polling)
 * ============================================================
 */

#include "types.h"
#include "peripherals.h"
#include "string.h"

/* ── Helper: Cache flush (ARM64) ─────────────────────────── */
static void data_memory_barrier(void)
{
    asm volatile("dmb sy" ::: "memory");
}

static void data_sync_barrier(void)
{
    asm volatile("dsb sy" ::: "memory");
}

/* ────────────────────────────────────────────────────────────
 *  mailbox_read
 *
 *  Reads a message from mailbox channel 8 (property tags).
 *  Blocks until a message is available.
 *  Result: 0 = success, 1 = error
 * ────────────────────────────────────────────────────────── */

static u32 mailbox_read(u8 channel)
{
    u32 mail_chan = (u32)channel;

    while (1) {
        /* Wait until mailbox is not empty */
        while (*MBOX_STATUS & MBOX_STATUS_EMPTY) {
            asm volatile("nop");
        }

        data_memory_barrier();

        /* Read message */
        u32 data = *MBOX_READ;

        /* Does the response belong to our channel? */
        if ((data & 0xF) == mail_chan) {
            return data >> 4;
        }
    }
}

/* ────────────────────────────────────────────────────────────
 *  mailbox_write
 *
 *  Sends a message to a mailbox channel.
 *  Blocks until the mailbox has space.
 * ────────────────────────────────────────────────────────── */

static void mailbox_write(u8 channel, u32 data)
{
    /* Data must be 16-byte-aligned, lower 4 bits = channel */
    if (data & 0xF) return;  /* Error: not aligned */

    u32 mail_chan = (u32)channel;

    /* Wait until mailbox is not full */
    while (*MBOX_STATUS & MBOX_STATUS_FULL) {
        asm volatile("nop");
    }

    data_sync_barrier();

    /* Send data + channel */
    *MBOX_WRITE = data | mail_chan;
}

/* ────────────────────────────────────────────────────────────
 *  mailbox_call
 *
 *  Sends a property tag message and waits for response.
 *  `buffer` must be 16-byte-aligned.
 *  Result: true = success, false = error
 * ────────────────────────────────────────────────────────── */

bool mailbox_call(u32 *buffer, u8 channel)
{

    /* Data must be 16-byte-aligned */
    if (!IS_ALIGNED((uintptr_t)buffer, 16)) {
        return false;
    }

    /* Write address + channel to mailbox */
    u32 addr = (u32)(uintptr_t)buffer;
    mailbox_write(channel, addr);

    /* Read response — GPU echoes back the buffer address.
     * The actual success/failure is in buffer[1]:
     *   0x80000000 = success, 0x80000001 = error */
    u32 result = mailbox_read(channel);
    (void)result;  /* Address echo, not the response code */

    /* Check the property tag response code in the buffer */
    return (buffer[1] == MBOX_RESPONSE);
}
