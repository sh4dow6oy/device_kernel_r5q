/* 
 * CRC32C
 *@Article{castagnoli-crc,
 * author =       { Guy Castagnoli and Stefan Braeuer and Martin Herrman},
 * title =        {{Optimization of Cyclic Redundancy-Check Codes with 24
 *                  and 32 Parity Bits}},
 * journal =      IEEE Transactions on Communication,
 * year =         {1993},
 * volume =       {41},
 * number =       {6},
 * pages =        {},
 * month =        {June},
 *}
 * Used by the iSCSI driver, possibly others, and derived from the
 * the iscsi-crc.c module of the linux-iscsi driver at
 * http://linux-iscsi.sourceforge.net.
 *
 * Following the example of lib/crc32, this function is intended to be
 * flexible and useful for all users.  Modules that currently have their
 * own crc32c, but hopefully may be able to use this one are:
 *  net/sctp (please add all your doco to here if you change to
 *            use this one!)
 *  <endoflist>
 *
 * Copyright (c) 2004 Cisco Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 */

#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/crc32c.h>
#include <linux/ratelimit.h>

static struct crypto_shash *tfm;

u32 crc32c(u32 crc, const void *address, unsigned int length)
{
	SHASH_DESC_ON_STACK(shash, tfm);
	u32 ret, *ctx;
	int err;

	if (unlikely(!tfm || IS_ERR(tfm))) {
		tfm = crypto_alloc_shash("crc32c", 0, 0);
		if (IS_ERR(tfm)) {
			pr_err_once("crc32c: tfm allocation failed, returning uncalculated CRC\n");
			return crc;
		}
	}

	ctx = (u32 *)shash_desc_ctx(shash);
	shash->tfm = tfm;
	shash->flags = 0;
	*ctx = crc;

	err = crypto_shash_update(shash, address, length);
	
	if (unlikely(err)) {
		pr_err_once("crc32c: crypto_shash_update failed with err %d\n", err);
		return crc;
	}

	ret = *ctx;
	barrier_data(ctx);
	return ret;
}

EXPORT_SYMBOL(crc32c);

static int __init libcrc32c_mod_init(void)
{
	tfm = crypto_alloc_shash("crc32c", 0, 0);
	if (IS_ERR(tfm)) {
		pr_err("libcrc32c: Failed to allocate crc32c transform: %ld\n", PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}
	return 0;
}

static void __exit libcrc32c_mod_fini(void)
{
	if (tfm && !IS_ERR(tfm))
		crypto_free_shash(tfm);
}

subsys_initcall(libcrc32c_mod_init);
module_exit(libcrc32c_mod_fini);

MODULE_AUTHOR("Clay Haapala <chaapala@cisco.com>");
MODULE_DESCRIPTION("CRC32c (Castagnoli) calculations");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: crc32c");
