/* tdfx_drv.c -- tdfx driver -*- linux-c -*-
 * Created: Thu Oct  7 10:38:32 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Daryll Strauss <daryll@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *
 * $FreeBSD: src/sys/dev/drm/tdfx_drv.c,v 1.2 2002/04/28 04:58:27 anholt Exp $
 */

#ifdef __linux__
#include <linux/config.h>
#endif /* __linux__ */

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/bus.h>
#include <pci/pcivar.h>
#endif /* __FreeBSD__ */

#include "dev/drm/tdfx.h"
#include "dev/drm/drmP.h"

#define DRIVER_AUTHOR		"VA Linux Systems Inc."

#define DRIVER_NAME		"tdfx"
#define DRIVER_DESC		"3dfx Banshee/Voodoo3+"
#define DRIVER_DATE		"20010216"

#define DRIVER_MAJOR		1
#define DRIVER_MINOR		0
#define DRIVER_PATCHLEVEL	0

#ifndef PCI_VENDOR_ID_3DFX
#define PCI_VENDOR_ID_3DFX 0x121A
#endif
#ifndef PCI_DEVICE_ID_3DFX_VOODOO5
#define PCI_DEVICE_ID_3DFX_VOODOO5 0x0009
#endif
#ifndef PCI_DEVICE_ID_3DFX_VOODOO4
#define PCI_DEVICE_ID_3DFX_VOODOO4 0x0007
#endif
#ifndef PCI_DEVICE_ID_3DFX_VOODOO3_3000 /* Voodoo3 3000 */
#define PCI_DEVICE_ID_3DFX_VOODOO3_3000 0x0005
#endif
#ifndef PCI_DEVICE_ID_3DFX_VOODOO3_2000 /* Voodoo3 3000 */
#define PCI_DEVICE_ID_3DFX_VOODOO3_2000 0x0004
#endif
#ifndef PCI_DEVICE_ID_3DFX_BANSHEE
#define PCI_DEVICE_ID_3DFX_BANSHEE 0x0003
#endif

#ifdef __FreeBSD__
/* List acquired from xc/xc/programs/Xserver/hw/xfree86/common/xf86PciInfo.h
 * Please report to eanholt@gladstone.uoregon.edu if your chip isn't
 * represented in the list or if the information is incorrect.
 */
drm_chipinfo_t DRM(devicelist)[] = {
	{0x121a, 0x0003, 1, "3dfx Voodoo Banshee"},
	{0x121a, 0x0004, 1, "3dfx Voodoo3 2000"},
	{0x121a, 0x0005, 1, "3dfx Voodoo3 3000"},
	{0x121a, 0x0007, 1, "3dfx Voodoo4"},
	{0x121a, 0x0009, 1, "3dfx Voodoo5"},
	{0, 0, 0, NULL}
};
#endif /* __FreeBSD__ */

#ifdef __linux__
/* For now, we'll only support multihead on Linux */
/* Uncomment this, and fixup drm_count_cards */
static drm_pci_list_t DRM(idlist)[] = {
	{ PCI_VENDOR_ID_3DFX, PCI_DEVICE_ID_3DFX_BANSHEE },
	{ PCI_VENDOR_ID_3DFX, PCI_DEVICE_ID_3DFX_VOODOO3_2000 },
	{ PCI_VENDOR_ID_3DFX, PCI_DEVICE_ID_3DFX_VOODOO3_3000 },
	{ PCI_VENDOR_ID_3DFX, PCI_DEVICE_ID_3DFX_VOODOO4 },
	{ PCI_VENDOR_ID_3DFX, PCI_DEVICE_ID_3DFX_VOODOO5 },
	{ 0, 0 }
};

#define DRIVER_CARD_LIST DRM(idlist)
#endif /* __linux__ */

#include "dev/drm/drm_auth.h"
#include "dev/drm/drm_bufs.h"
#include "dev/drm/drm_context.h"
#include "dev/drm/drm_dma.h"
#include "dev/drm/drm_drawable.h"
#include "dev/drm/drm_drv.h"

#ifdef __linux__
#ifndef MODULE
/* DRM(options) is called by the kernel to parse command-line options
 * passed via the boot-loader (e.g., LILO).  It calls the insmod option
 * routine, drm_parse_drm.
 */

/* JH- We have to hand expand the string ourselves because of the cpp.  If
 * anyone can think of a way that we can fit into the __setup macro without
 * changing it, then please send the solution my way.
 */
static int __init tdfx_options( char *str )
{
	DRM(parse_options)( str );
	return 1;
}

__setup( DRIVER_NAME "=", tdfx_options );
#endif
#endif /* __linux__ */

#include "dev/drm/drm_fops.h"
#include "dev/drm/drm_init.h"
#include "dev/drm/drm_ioctl.h"
#include "dev/drm/drm_lock.h"
#include "dev/drm/drm_memory.h"
#ifdef __linux__
#include "dev/drm/drm_proc.h"
#endif /* __linux__ */
#include "dev/drm/drm_vm.h"
#ifdef __linux__
#include "dev/drm/drm_stub.h"
#endif /* __linux__ */
#ifdef __FreeBSD__
#include "dev/drm/drm_sysctl.h"

DRIVER_MODULE(tdfx, pci, tdfx_driver, tdfx_devclass, 0, 0);
#endif /* __FreeBSD__ */
