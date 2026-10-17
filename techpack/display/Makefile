# SPDX-License-Identifier: GPL-2.0-only

# Include-uri globale pentru tot Techpack Display
subdir-ccflags-y += -I$(srctree)/techpack/display/msm
subdir-ccflags-y += -I$(srctree)/techpack/display/msm/sde
subdir-ccflags-y += -I$(srctree)/techpack/display/msm/dsi
subdir-ccflags-y += -I$(srctree)/techpack/display/msm/dp
subdir-ccflags-y += -I$(srctree)/techpack/display/msm/samsung
subdir-ccflags-y += -I$(srctree)/techpack/display/rotator
subdir-ccflags-y += -I$(srctree)/drivers/gpu/drm/msm
subdir-ccflags-y += -I$(srctree)/drivers/gpu/drm/msm/hdmi

LINUXINCLUDE += -I$(srctree)/techpack/display/msm \
		-I$(srctree)/techpack/display/msm/sde \
		-I$(srctree)/techpack/display/msm/dsi \
		-I$(srctree)/techpack/display/msm/dp \
		-I$(srctree)/drivers/gpu/drm/msm/hdmi

# auto-detect subdirs
ifeq ($(CONFIG_ARCH_KONA), y)
include $(srctree)/techpack/display/config/konadisp.conf
endif

ifeq ($(CONFIG_ARCH_KONA), y)
LINUXINCLUDE    += -include $(srctree)/techpack/display/config/konadispconf.h
endif

ifeq ($(CONFIG_ARCH_LITO), y)
include $(srctree)/techpack/display/config/saipdisp.conf
endif

ifeq ($(CONFIG_ARCH_LITO), y)
LINUXINCLUDE    += -include $(srctree)/techpack/display/config/saipdispconf.h
endif

ifeq ($(CONFIG_ARCH_BENGAL), y)
include $(srctree)/techpack/display/config/bengaldisp.conf
endif

ifeq ($(CONFIG_ARCH_BENGAL), y)
LINUXINCLUDE    += -include $(srctree)/techpack/display/config/bengaldispconf.h
endif

ifeq ($(CONFIG_ARCH_SM8150), y)
include $(srctree)/techpack/display/config/msmniledisp.conf
endif

ifeq ($(CONFIG_ARCH_SM8150), y)
LINUXINCLUDE    += -include $(srctree)/techpack/display/config/msmniledispconf.h
endif

obj-$(CONFIG_DRM_MSM) += msm/
obj-$(CONFIG_MSM_SDE_ROTATOR) += rotator/
obj-$(CONFIG_QCOM_MDSS_PLL) += pll/
