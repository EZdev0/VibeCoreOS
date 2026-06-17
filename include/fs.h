/*
 * ============================================================
 *  VibeCore OS — Filesystem Header
 * ============================================================
 */

#ifndef _FS_H
#define _FS_H

#include "types.h"

void fs_init(void);
bool fs_mount(u8 *boot_sector);
int  fs_list_dir(const char *path);
int  fs_create_file(const char *name);
int  fs_read_file(const char *path, u8 *buffer, size_t size);
int  fs_write_file(const char *path, const u8 *data, size_t size);
u64  fs_get_free_space(void);
void fs_unmount(void);

#endif /* _FS_H */
int fs_create_dir(const char *name);
void fs_lost_and_found_recover(void);
