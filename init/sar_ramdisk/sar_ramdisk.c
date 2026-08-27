#include "bootimg.h"
#include "../do_mounts.h"

#include <linux/syscalls.h>
#include <linux/slab.h>

extern void clean_rootfs(void);
extern void flush_delayed_fput(void);
extern char* unpack_to_rootfs(char *buf, unsigned long len);

static int padding(unsigned itemsize, int pagesize) {
	unsigned pagemask = pagesize - 1;

	if ((itemsize & pagemask) == 0)
		return 0;

	return pagesize - (itemsize & pagemask);
}

__init int mount_sar_ramdisk(char* name) {
	struct boot_img_hdr_v2 header; /* Folosește structura V2 */
	unsigned int rd_offset = 0;    /* CRITIC: Inițializare cu 0 */
	int fd;
	int res = 0;
	char* buf;

	fd = sys_open(name, O_RDONLY, 0);

	if (fd < 0) {
		pr_err("SAR_RD: Failed to open %s: Error %d\n", name, fd);
		return 0;
	}

	if (sys_read(fd, (char*) &header, sizeof(header)) != sizeof(header)) {
		pr_err("SAR_RD: Failed to read bootimage header\n");
		goto clean_nobuf;
	}

	/* Validare Magic Android */
	if (memcmp(header.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
		pr_err("SAR_RD: Invalid BOOT_MAGIC!\n");
		goto clean_nobuf;
	}

	/*
	 * CALCUL CORECorect OFFSET RAMDISK (Android Boot Header Spec):
	 * Offset 0: Header (ocupă exact 1 x page_size)
	 * Offset 1: Kernel (ocupă kernel_size + padding de pagină)
	 * Offset 2: Ramdisk (AICI TREBUIE SĂ AJUNGEM)
	 */
	rd_offset = header.page_size; /* Header alignment */
	rd_offset += header.kernel_size;
	rd_offset += padding(header.kernel_size, header.page_size);

	pr_info("SAR_RD: Trying to load Ramdisk at offset %u (Size: %u)\n", rd_offset, header.ramdisk_size);

	if (sys_lseek(fd, rd_offset, 0) != rd_offset) {
		pr_err("SAR_RD: Failed to seek to %u\n", rd_offset);
		goto clean_nobuf;
	}

	if (header.ramdisk_size == 0) {
		pr_err("SAR_RD: Ramdisk size is 0 in header!\n");
		goto clean_nobuf;
	}

	buf = kmalloc(header.ramdisk_size, GFP_KERNEL);

	if (!buf) {
		pr_err("SAR_RD: Out of memory trying to allocate %u bytes\n", header.ramdisk_size);
		goto clean_nobuf;
	}

	if (sys_read(fd, buf, header.ramdisk_size) != header.ramdisk_size) {
		pr_err("SAR_RD: EOF while trying to read ramdisk!\n");
		goto clean;
	}

	clean_rootfs();
	res = !unpack_to_rootfs(buf, header.ramdisk_size);
	flush_delayed_fput();
	
#ifdef CONFIG_BLK_DEV_INITRD
	load_default_modules();
#endif

clean:
	kfree(buf);
clean_nobuf:
	sys_close(fd);

	if (res)
		pr_info("SAR_RD: Successfully loaded ramdisk\n");
	else
		pr_err("SAR_RD: Failed to load ramdisk\n");

	return res;
}
