#ifndef ANDROID_BOOTIMG_H
#define ANDROID_BOOTIMG_H

#include <stdint.h>

#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024

/* Boot Image Header Version 2 (Android 10+ SAR / SM8150) */
struct boot_img_hdr_v2 {
    // Must be BOOT_MAGIC ("ANDROID!")
    uint8_t magic[BOOT_MAGIC_SIZE];

    uint32_t kernel_size;  /* Dimensiunea kernel-ului (Image.gz) în bytes */
    uint32_t kernel_addr;  /* Adresa fizică de încărcare (ex: 0x00008000) */

    uint32_t ramdisk_size; /* Dimensiunea ramdisk-ului în bytes */
    uint32_t ramdisk_addr; /* Adresa fizică ramdisk (ex: 0x02000000) */

    uint32_t second_size;  /* Size = 0 pe SM8150 */
    uint32_t second_addr;  /* Load addr secundar */

    uint32_t tags_addr;    /* Adresa fizică pentru kernel tags (ex: 0x00000100) */
    uint32_t page_size;    /* Dimensiunea paginii pe flash (ex: 2048) */

    // În Header V2, acest câmp trebuie să fie fix 2
    uint32_t header_version; 

    // OS version și Security Patch Level
    uint32_t os_version;

    uint8_t name[BOOT_NAME_SIZE]; /* Nume produs / gol */

    uint8_t cmdline[BOOT_ARGS_SIZE]; /* Parametrii kernel transmisi de ABL */

    uint32_t id[8]; /* Hash SHA-1 / checksum al imaginii */

    // Supplemental command line data
    uint8_t extra_cmdline[BOOT_EXTRA_ARGS_SIZE];

    /* Câmpuri specifice Header V1 / V2 */
    uint32_t recovery_dtbo_size;   /* Dimensiunea DTBO recovery în bytes */
    uint64_t recovery_dtbo_offset; /* Offset-ul absolut către DTBO în imagine */
    uint32_t header_size;          /* Dimensiunea totală a antetului V2 (1660 bytes) */

    /* --- Adăugare critică pentru HEADER V2 --- */
    uint32_t dtb_size;             /* Dimensiunea DTB-ului principal (SM8150) */
    uint64_t dtb_addr;             /* Adresa fizică de încărcare în RAM a DTB-ului */
} __attribute__((packed));

#endif // ANDROID_BOOTIMG_H
