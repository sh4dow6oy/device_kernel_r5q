# SPDX-License-Identifier: GPL-2.0-only

# auto-detect subdirs
ifeq ($(CONFIG_ARCH_SM8150), y)
include $(srctree)/techpack/camera-msmnile/config/msmnilecamera.conf
endif

ifeq ($(CONFIG_ARCH_SM8150), y)
LINUXINCLUDE    += \
		-include $(srctree)/techpack/camera-msmnile/config/msmnilecameraconf.h
endif

ifdef CONFIG_SPECTRA_CAMERA
# Use USERINCLUDE when you must reference the UAPI directories only.
USERINCLUDE     += \
                -I$(srctree)/techpack/sec-camera/include/uapi

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
LINUXINCLUDE    += \
                -I$(srctree)/techpack/sec-camera/include/uapi \
                -I$(srctree)/techpack/sec-camera/include
obj-y += drivers/
else
$(info Target not found)
endif
