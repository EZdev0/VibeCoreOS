/*
 * ============================================================
 *  VibeCore OS — Recovery System Header
 * ============================================================
 */

#ifndef _RECOVERY_H
#define _RECOVERY_H

#include "types.h"

/* ── Journal ──────────────────────────────────────────────── */
void recovery_journal_init(void);

/* ── Trash ────────────────────────────────────────────────── */
void recovery_trash_init(void);
int  recovery_trash_delete(const char *path);
int  recovery_trash_restore(const char *trash_filename);

/* ── Versioning ───────────────────────────────────────────── */
void recovery_versions_init(void);
int  recovery_version_save(const char *path);
int  recovery_version_restore(const char *version_path, const char *target_path);

/* ── CRC32 Auto-Verify ────────────────────────────────────── */
void recovery_hashes_init(void);
void recovery_hash_store(const char *path, u32 crc);
bool recovery_hash_verify(const char *path, const u8 *data, size_t size);

/* ── Boot Recovery ────────────────────────────────────────── */
void recovery_boot_scan(void);

/* ── System File Protection ──────────────────────────────── */
bool recovery_is_protected(const char *path);
int  recovery_protect_add(const char *path);

/* ── Snapshots ────────────────────────────────────────────── */
void recovery_snapshots_init(void);
void recovery_snapshot_create(void);
void recovery_snapshot_restore(void);

/* ── Auto-Recovery ────────────────────────────────────────── */
void recovery_trigger_auto(const char *reason);

/* ── Shell Integration ───────────────────────────────────── */
void recovery_shell_fsck(void);
void recovery_shell_trash_list(void);
void recovery_shell_versions_list(const char *path);

#endif /* _RECOVERY_H */
