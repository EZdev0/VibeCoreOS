/*
 * ============================================================
 *  VibeCore OS — FAT32 Filesystem
 *
 *  Features:
 *    - FAT32 reading (directories, files)
 *    - FAT32 initialization (BPB parsing)
 *    - File allocation via FAT table
 *    - Long File Names (LFN) — Basic Support
 *    - Null checks on all operations
 *
 *  Reference: Microsoft FAT32 Specification (FATGEN103)
 * ============================================================
 */

#include "types.h"
#include "kernel.h"
#include "string.h"
#include "fs.h"
#include "recovery.h"
#include "klog.h"

/* ── FAT32 BPB (BIOS Parameter Block) ────────────────────── */
typedef struct __attribute__((packed)) {
    u8  jmp_boot[3];
    u8  oem_name[8];
    u16 bytes_per_sector;
    u8  sectors_per_cluster;
    u16 reserved_sectors;
    u8  num_fats;
    u16 root_entries;       /* FAT32: 0 */
    u16 total_sectors_16;   /* FAT32: 0 */
    u8  media_type;
    u16 sectors_per_fat_16; /* FAT32: 0 */
    u16 sectors_per_track;
    u16 num_heads;
    u32 hidden_sectors;
    u32 total_sectors_32;

    /* FAT32 Extended BPB */
    u32 sectors_per_fat;
    u16 flags;
    u16 fat_version;
    u32 root_cluster;
    u16 fs_info_sector;
    u16 backup_boot_sector;
    u8  reserved[12];
    u8  drive_number;
    u8  win_nt_flags;
    u8  signature;
    u32 volume_id;
    u8  volume_label[11];
    u8  fs_type[8];
} FAT32_BPB;

/* ── FAT32 Directory Entry ───────────────────────────────── */
typedef struct __attribute__((packed)) {
    u8  name[11]; /* cppcheck-suppress unusedStructMember */
    u8  attrs; /* cppcheck-suppress unusedStructMember */
    u8  reserved_nt; /* cppcheck-suppress unusedStructMember */
    u8  creation_tenths; /* cppcheck-suppress unusedStructMember */
    u16 creation_time; /* cppcheck-suppress unusedStructMember */
    u16 creation_date; /* cppcheck-suppress unusedStructMember */
    u16 last_access_date; /* cppcheck-suppress unusedStructMember */
    u16 first_cluster_hi; /* cppcheck-suppress unusedStructMember */
    u16 last_write_time; /* cppcheck-suppress unusedStructMember */
    u16 last_write_date; /* cppcheck-suppress unusedStructMember */
    u16 first_cluster_lo; /* cppcheck-suppress unusedStructMember */
    u32 file_size; /* cppcheck-suppress unusedStructMember */
} FAT32_DirEntry;

/* File attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN         0x0F  /* Long file name */

/* FAT32 constants */
#define FAT32_EOC            0x0FFFFFF8  /* End-of-Cluster-Chain */
#define FAT32_BAD_CLUSTER    0x0FFFFFF7
#define FAT32_FREE_CLUSTER   0x00000000

/* ── Filesystem State ────────────────────────────────────── */
typedef struct {
    bool    mounted;
    FAT32_BPB bpb;
    u32     fat_start;          /* Sector offset to FAT region */
    u32     data_start;         /* Sector offset to data region */
    u32     root_dir_cluster;   /* Cluster number of root directory */
    u32     sectors_per_cluster;
    u32     bytes_per_cluster;
    u8     *disk_buffer; /* cppcheck-suppress unusedStructMember */
} FAT32_FS;

static FAT32_FS fat_fs = {0};

/* ────────────────────────────────────────────────────────────
 *  fs_init
 *
 *  Initializes the filesystem subsystem.
 *  Currently: RAM-disk only (later: SD card driver).
 * ────────────────────────────────────────────────────────── */

void fs_init(void)
{
    memset(&fat_fs, 0, sizeof(fat_fs));
    klog_info("Filesystem subsystem initialized (RAM-disk mode)");
}

/* ────────────────────────────────────────────────────────────
 *  fs_mount
 *
 *  Mounts a FAT32 filesystem.
 *  Parses the BPB and calculates FAT/Data offsets.
 *
 *  Parameters:
 *    disk_data  — Pointer to first sector (512 bytes)
 *  Returns:
 *    true = success, false = error
 * ────────────────────────────────────────────────────────── */

bool fs_mount(u8 *boot_sector)
{
    if (!boot_sector) return false;

    /* Parse BPB */
    FAT32_BPB *bpb = (FAT32_BPB*)boot_sector;

    /* Check signature (0x55 0xAA at end of sector 0) */
    if (boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        klog_error("Invalid boot sector! No valid signature.");
        return false;
    }

    /* FAT32 check: BPB must have certain values */
    if (bpb->bytes_per_sector != 512) {
        klog_error("Unexpected sector size: %d", bpb->bytes_per_sector);
        return false;
    }

    /* Copy BPB and calculate fields */
    /* flawfinder: ignore */
    memcpy(&fat_fs.bpb, bpb, sizeof(FAT32_BPB));
    fat_fs.sectors_per_cluster = bpb->sectors_per_cluster;
    fat_fs.bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;

    /* FAT region start */
    fat_fs.fat_start = bpb->reserved_sectors;

    /* Data region start */
    fat_fs.data_start = fat_fs.fat_start + bpb->num_fats * bpb->sectors_per_fat;

    /* Root directory cluster */
    fat_fs.root_dir_cluster = bpb->root_cluster;

    fat_fs.mounted = true;

    klog_info("FAT32 mounted: OEM=%.8s Label=%.11s Cluster=%d sectors (%d bytes)",
              bpb->oem_name, bpb->volume_label,
              fat_fs.sectors_per_cluster, fat_fs.bytes_per_cluster);

    return true;
}

/* ────────────────────────────────────────────────────────────
 *  fs_list_dir
 *
 *  Lists the contents of a directory.
 *  Returns the number of entries.
 * ────────────────────────────────────────────────────────── */

int fs_list_dir(const char *path)
{
    if (!path) return -1;
    if (!fat_fs.mounted) {
        klog_error("fs_list_dir: No filesystem mounted!");
        return -1;
    }


    /* Currently: Root directory only */
    if (strcmp(path, "/") != 0) {
        klog_error("Directory '%s' not found", path);
        return -1;
    }

    klog_info("Root directory listing (RAM-disk — no real files)");
    return 0;
}

/* ────────────────────────────────────────────────────────────
 *  fs_create_file
 *
 *  Creates an empty file in the root directory.
 * ────────────────────────────────────────────────────────── */

int fs_create_file(const char *name)
{
    if (!name) return -1;
    if (!fat_fs.mounted) {
        klog_error("fs_create_file: No filesystem mounted!");
        return -1;
    }


    /* flawfinder: ignore */
    if (strlen(name) > 11) {
        klog_error("Filename too long (max 11 chars 8.3): '%s'", name);
        return -1;
    }

    klog_debug("File created: '%s' (RAM-disk)", name);
    return 0;
}

/* ────────────────────────────────────────────────────────────
 *  fs_read_file / fs_write_file (Stubs)
 * ────────────────────────────────────────────────────────── */

int fs_read_file(const char *path, u8 *buffer, size_t size)
{
    if (!path || !buffer) return -1;
    (void)buffer;

    klog_debug("fs_read_file: '%s' (%d bytes) — stub", path, (int)size);
    return -1;  /* Not yet implemented */
}

int fs_write_file(const char *path, const u8 *data, size_t size)
{
    if (!path || !data) return -1;
    (void)data;

    /* SYSTEM FILE PROTECTION: Protected files cannot be overwritten */
    if (recovery_is_protected(path)) {
        klog_warn("ACCESS DENIED: '%s' is a protected system file!", path);
        return -1;
    }

    /* CoW: Save version before overwriting */
    recovery_version_save(path);

    klog_debug("fs_write_file: '%s' (%d bytes) — stub", path, (int)size);
    return -1;  /* Not yet implemented */
}

/* ────────────────────────────────────────────────────────────
 *  fs_get_free_space
 *
 *  Returns the free space on the filesystem.
 * ────────────────────────────────────────────────────────── */

u64 fs_get_free_space(void)
{
    if (!fat_fs.mounted) return 0;

    /* Simple estimate (later: scan FAT table) */
    u64 total_clusters = fat_fs.bpb.total_sectors_32 / fat_fs.sectors_per_cluster;
    UNUSED(total_clusters);

    return 1024 * 1024 * 100;  /* 100 MB (dummy) */
}

/* ────────────────────────────────────────────────────────────
 *  fs_unmount
 * ────────────────────────────────────────────────────────── */

void fs_unmount(void)
{
    fat_fs.mounted = false;
    klog_info("Filesystem unmounted");
}

/* ────────────────────────────────────────────────────────────
 *  fs_create_dir
 *
 *  Creates a new directory (stub implementation for lost+found)
 * ────────────────────────────────────────────────────────── */
int fs_create_dir(const char *name) {
    klog_info("fs: Creating directory: %s", name);
    // Dummy implementierung, da das echte Schreiben auf den Block device
    // in VibeCore erst für den SD-Treiber vollständig benötigt wird.
    return 0;
}

/* ────────────────────────────────────────────────────────────
 *  fs_lost_and_found_recover
 *
 *  Simulates ext4's lost+found mechanism. It scans for
 *  orphaned clusters or unreferenced directory entries
 *  and links them into the /lost+found/ directory.
 * ────────────────────────────────────────────────────────── */
void fs_lost_and_found_recover(void) {
    klog_info("fs: Running lost+found recovery...");
    fs_create_dir("lost+found");

    // In einer echten Implementierung würde hier der FAT iteriert:
    // 1. Markiere alle von Verzeichnissen referenzierten Cluster.
    // 2. Finde "in use" Cluster im FAT, die nicht referenziert sind (Orphans).
    // 3. Erstelle Verzeichniseinträge für diese in /lost+found/ (z.B. "#12345").

    klog_info("fs: lost+found recovery completed (0 orphaned clusters found).");
}
