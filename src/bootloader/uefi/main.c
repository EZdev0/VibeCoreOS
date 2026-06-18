typedef unsigned short CHAR16;
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (*EFI_TEXT_STRING)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);

/* UEFI structs: minimal definitions, preserving field offsets for ABI compatibility.
   Only OutputString (offset +8) and ConOut (offset +64) are accessed at runtime. */
typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void *_pad0;               /* Reset          (offset +0) */
    EFI_TEXT_STRING OutputString; /*              (offset +8) */
    void *_pad1[8];            /* TestString … Mode (offset +16 … +72) */
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    unsigned long long Signature;
    unsigned int Revision;
    unsigned int HeaderSize;
    unsigned int CRC32;
    unsigned int Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    EFI_TABLE_HEADER Hdr;                      /* (offset +0) */
    void *_pad0;                               /* FirmwareVendor   */
    unsigned int _pad1;                        /* FirmwareRevision  */
    unsigned int _pad2;                        /* (alignment pad)   */
    void *_pad3[3];                            /* ConIn/Out handles */
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;    /* (offset +64)     */
} EFI_SYSTEM_TABLE;

#define EFI_SUCCESS 0

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    (void)ImageHandle;

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16 *)L"VibeCore OS Booting via Custom UEFI Stub...\r\n");
    // Since direct jump to kernel8.img requires proper memory mapping in the stub,
    // we'll print a message and hang, confirming our stub executed.
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16 *)L"[SUCCESS] Stub Loaded!\r\n");

    while (1) {}
    return EFI_SUCCESS;
}
