typedef unsigned short CHAR16;
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (*EFI_TEXT_STRING)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {

    EFI_TEXT_STRING OutputString;
    // We omit the rest for the minimal stub
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {





} EFI_TABLE_HEADER;

typedef struct {






    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
} EFI_SYSTEM_TABLE;

#define EFI_SUCCESS 0

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16 *)L"VibeCore OS UEFI Stub Booting...\r\n");
    while (1) {}
    return EFI_SUCCESS;
}
