/*	$OpenBSD: usbdevs_data.h,v 1.34 2001/10/02 01:38:38 millert Exp $	*/

/*
 * THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	OpenBSD: usbdevs,v 1.35 2001/10/02 01:37:36 millert Exp 
 */
/* $NetBSD: usbdevs,v 1.127 2000/11/15 14:36:09 augustss Exp $ */

/*
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

const struct usb_known_product usb_known_products[] = {
	{
	    USB_VENDOR_3COM, USB_PRODUCT_3COM_HOMECONN,
	    "HomeConnect Camera",
	},
	{
	    USB_VENDOR_3COM, USB_PRODUCT_3COM_3C19250,
	    "3C19250 Ethernet",
	},
	{
	    USB_VENDOR_3COM, USB_PRODUCT_3COM_USR56K,
	    "U.S.Robotics 56000",
	},
	{
	    USB_VENDOR_3COM, USB_PRODUCT_3COM_3C460,
	    "HomeConnect Ethernet",
	},
	{
	    USB_VENDOR_3COMUSR, USB_PRODUCT_3COMUSR_OFFICECONN,
	    "3Com OfficeConnect Analog Modem",
	},
	{
	    USB_VENDOR_3COMUSR, USB_PRODUCT_3COMUSR_USRISDN,
	    "3Com U.S. Robotics Pro ISDN TA",
	},
	{
	    USB_VENDOR_3COMUSR, USB_PRODUCT_3COMUSR_HOMECONN,
	    "3Com HomeConnect camera",
	},
	{
	    USB_VENDOR_3COMUSR, USB_PRODUCT_3COMUSR_USR56K,
	    "U.S.Robotics 56000",
	},
	{
	    USB_VENDOR_ABOCOM, USB_PRODUCT_ABOCOM_URE450,
	    "USB Ethernet",
	},
	{
	    USB_VENDOR_ACERP, USB_PRODUCT_ACERP_ACERSCAN_C310U,
	    "Acerscan C310U",
	},
	{
	    USB_VENDOR_ACERP, USB_PRODUCT_ACERP_ACERSCAN_320U,
	    "Acerscan 320U",
	},
	{
	    USB_VENDOR_ACERP, USB_PRODUCT_ACERP_ACERSCAN_640U,
	    "Acerscan 640U",
	},
	{
	    USB_VENDOR_ACERP, USB_PRODUCT_ACERP_ACERSCAN_620U,
	    "Acerscan 620U",
	},
	{
	    USB_VENDOR_ACTIVEWIRE, USB_PRODUCT_ACTIVEWIRE_IOBOARD,
	    "I/O Board",
	},
	{
	    USB_VENDOR_ACTIVEWIRE, USB_PRODUCT_ACTIVEWIRE_IOBOARD_FW1,
	    "I/O Board, rev. 1",
	},
	{
	    USB_VENDOR_ADMTEK, USB_PRODUCT_ADMTEK_PEGASUS,
	    "AN986 Ethernet",
	},
	{
	    USB_VENDOR_ADS, USB_PRODUCT_ADS_UBS10BT,
	    "UBS-10BT Ethernet",
	},
	{
	    USB_VENDOR_ELECOM, USB_PRODUCT_ELECOM_MOUSE29UO,
	    "mouse 29UO",
	},
	{
	    USB_VENDOR_AGFA, USB_PRODUCT_AGFA_SNAPSCAN1212U,
	    "SnapScan 1212U",
	},
	{
	    USB_VENDOR_AGFA, USB_PRODUCT_AGFA_SNAPSCANTOUCH,
	    "SnapScan Touch",
	},
	{
	    USB_VENDOR_AGFA, USB_PRODUCT_AGFA_SNAPSCAN1212U2,
	    "SnapScan 1212U",
	},
	{
	    USB_VENDOR_AKS, USB_PRODUCT_AKS_USBHASP,
	    "USB-HASP 0.06",
	},
	{
	    USB_VENDOR_ALCOR2, USB_PRODUCT_ALCOR2_KBD_HUB,
	    "Kbd Hub",
	},
	{
	    USB_VENDOR_ALCOR, USB_PRODUCT_ALCOR_MA_KBD_HUB,
	    "MacAlly Kbd Hub",
	},
	{
	    USB_VENDOR_ALCOR, USB_PRODUCT_ALCOR_AU9814,
	    "AU9814 Hub",
	},
	{
	    USB_VENDOR_ALCOR, USB_PRODUCT_ALCOR_SM_KBD,
	    "MicroConnectors/StrongMan",
	},
	{
	    USB_VENDOR_ALCOR, USB_PRODUCT_ALCOR_NEC_KBD_HUB,
	    "NEC Kbd Hub",
	},
	{
	    USB_VENDOR_ALTEC, USB_PRODUCT_ALTEC_ADA70,
	    "ADA70 Speakers",
	},
	{
	    USB_VENDOR_ALTEC, USB_PRODUCT_ALTEC_ASC495,
	    "ASC495 Speakers",
	},
	{
	    USB_VENDOR_APC, USB_PRODUCT_APC_UPSPRO500,
	    "Back-UPS Pro 500",
	},
	{
	    USB_VENDOR_ANCHOR, USB_PRODUCT_ANCHOR_EZUSB,
	    "EZUSB",
	},
	{
	    USB_VENDOR_ANCHOR, USB_PRODUCT_ANCHOR_EZLINK,
	    "EZLINK",
	},
	{
	    USB_VENDOR_AOX, USB_PRODUCT_AOX_USB101,
	    "USB Ethernet controller engine",
	},
	{
	    USB_VENDOR_APPLE, USB_PRODUCT_APPLE_OPTMOUSE,
	    "Optical mouse",
	},
	{
	    USB_VENDOR_APPLE, USB_PRODUCT_APPLE_SPEAKERS,
	    "Speakers",
	},
	{
	    USB_VENDOR_ATEN, USB_PRODUCT_ATEN_UC1284,
	    "Parallel printer adapter",
	},
	{
	    USB_VENDOR_ATEN, USB_PRODUCT_ATEN_UC10T,
	    "10Mbps Ethernet",
	},
	{
	    USB_VENDOR_ATEN, USB_PRODUCT_ATEN_UC232A,
	    "Serial adapter",
	},
	{
	    USB_VENDOR_ATMEL, USB_PRODUCT_ATMEL_UHB124,
	    "UHB124 hub",
	},
	{
	    USB_VENDOR_AVISION, USB_PRODUCT_AVISION_1200U,
	    "1200U scanner",
	},
	{
	    USB_VENDOR_BELKIN2, USB_PRODUCT_BELKIN2_F5U002,
	    "F5U002 Parallel printer adapter",
	},
	{
	    USB_VENDOR_BILLIONTON, USB_PRODUCT_BILLIONTON_USB100,
	    "USB100N 10/100 Ethernet",
	},
	{
	    USB_VENDOR_BROTHER, USB_PRODUCT_BROTHER_HL1050,
	    "HL-1050 laser printer",
	},
	{
	    USB_VENDOR_BTC, USB_PRODUCT_BTC_BTC7932,
	    "Keyboard",
	},
	{
	    USB_VENDOR_CANON, USB_PRODUCT_CANON_S10,
	    "PowerShot S10",
	},
	{
	    USB_VENDOR_CATC, USB_PRODUCT_CATC_NETMATE,
	    "Netmate Ethernet",
	},
	{
	    USB_VENDOR_CATC, USB_PRODUCT_CATC_NETMATE2,
	    "Netmate2 Ethernet",
	},
	{
	    USB_VENDOR_CATC, USB_PRODUCT_CATC_CHIEF,
	    "USB Chief Bus & Protocol Analyzer",
	},
	{
	    USB_VENDOR_CATC, USB_PRODUCT_CATC_ANDROMEDA,
	    "Andromeda hub",
	},
	{
	    USB_VENDOR_CHERRY, USB_PRODUCT_CHERRY_MY3000KBD,
	    "My3000",
	},
	{
	    USB_VENDOR_CHERRY, USB_PRODUCT_CHERRY_MY3000HUB,
	    "My3000",
	},
	{
	    USB_VENDOR_CHIC, USB_PRODUCT_CHIC_MOUSE1,
	    "mouse",
	},
	{
	    USB_VENDOR_CHIC, USB_PRODUCT_CHIC_CYPRESS,
	    "Cypress",
	},
	{
	    USB_VENDOR_CHICONY, USB_PRODUCT_CHICONY_KB8933,
	    "KB-8933",
	},
	{
	    USB_VENDOR_COMPAQ, USB_PRODUCT_COMPAQ_PJB100,
	    "Personal Jukebox PJB100",
	},
	{
	    USB_VENDOR_CONNECTIX, USB_PRODUCT_CONNECTIX_QUICKCAM,
	    "QuickCam",
	},
	{
	    USB_VENDOR_COREGA, USB_PRODUCT_COREGA_ETHER_USB_T,
	    "Ether USB-T",
	},
	{
	    USB_VENDOR_COREGA, USB_PRODUCT_COREGA_FETHER_USB_TX,
	    "FEther USB-TX",
	},
	{
	    USB_VENDOR_CREATIVELABS, USB_PRODUCT_CREATIVELABS_NOMAD_II,
	    "Nomad II",
	},
	{
	    USB_VENDOR_CREATIVELABS, USB_PRODUCT_CREATIVELABS_NOMAD_IIMG,
	    "Nomad II MG",
	},
	{
	    USB_VENDOR_CTX, USB_PRODUCT_CTX_EX1300,
	    "Ex1300 hub",
	},
	{
	    USB_VENDOR_CYPRESS, USB_PRODUCT_CYPRESS_MOUSE,
	    "mouse",
	},
	{
	    USB_VENDOR_CYPRESS, USB_PRODUCT_CYPRESS_THERMO,
	    "thermometer",
	},
	{
	    USB_VENDOR_CYPRESS, USB_PRODUCT_CYPRESS_FMRADIO,
	    "FM Radio",
	},
	{
	    USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DSB650C,
	    "10Mbps Ethernet",
	},
	{
	    USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DSB650TX,
	    "10/100 Ethernet",
	},
	{
	    USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DSB650TX_PNA,
	    "1/10/100 Ethernet",
	},
	{
	    USB_VENDOR_DLINK, USB_PRODUCT_DLINK_DSB650,
	    "10/100 Ethernet",
	},
	{
	    USB_VENDOR_DALLAS, USB_PRODUCT_DALLAS_J6502,
	    "J-6502 speakers",
	},
	{
	    USB_VENDOR_DIAMOND, USB_PRODUCT_DIAMOND_RIO500USB,
	    "Rio 500 USB",
	},
	{
	    USB_VENDOR_DIGI, USB_PRODUCT_DIGI_ACCELEPORT2,
	    "AccelePort 2",
	},
	{
	    USB_VENDOR_DIGI, USB_PRODUCT_DIGI_ACCELEPORT4,
	    "AccelePort 4",
	},
	{
	    USB_VENDOR_DIGI, USB_PRODUCT_DIGI_ACCELEPORT8,
	    "AccelePort 8",
	},
	{
	    USB_VENDOR_EIZO, USB_PRODUCT_EIZO_HUB,
	    "hub",
	},
	{
	    USB_VENDOR_EIZO, USB_PRODUCT_EIZO_MONITOR,
	    "monitor",
	},
	{
	    USB_VENDOR_ELSA, USB_PRODUCT_ELSA_MODEM1,
	    "ELSA",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_1S,
	    "1S serial connector",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_2S,
	    "2S serial connector",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_1S25,
	    "1S25 serial connector",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_4S,
	    "4S serial connector",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_E45,
	    "E45 Ethernet",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_CENTRONICS,
	    "Centronics connector",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_1S9,
	    "1S9 serial connector",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_EZUSB,
	    "EZ-USB",
	},
	{
	    USB_VENDOR_ENTREGA, USB_PRODUCT_ENTREGA_2U4S,
	    "2U4S serial connector/usb hub",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_PRINTER1,
	    "USB Printer",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_PRINTER2,
	    "ISD Smart Cable for Mac",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_PRINTER3,
	    "ISD Smart Cable",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_PRINTER5,
	    "USB Printer",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_636,
	    "Perfection 636U / 636Photo scanner",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_610,
	    "Perfection 610 scanner",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_1200,
	    "Perfection 1200U / 1200Photo scanner",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_1600,
	    "Expression 1600 scanner",
	},
	{
	    USB_VENDOR_EPSON, USB_PRODUCT_EPSON_1640,
	    "Expression 1640SU scanner",
	},
	{
	    USB_VENDOR_ETEK, USB_PRODUCT_ETEK_1COM,
	    "Serial port",
	},
	{
	    USB_VENDOR_EXTENDED, USB_PRODUCT_EXTENDED_XTNDACCESS,
	    "XTNDAccess IrDA",
	},
	{
	    USB_VENDOR_GRAVIS, USB_PRODUCT_GRAVIS_GAMEPADPRO,
	    "GamePad Pro",
	},
	{
	    USB_VENDOR_GRIFFIN, USB_PRODUCT_GRIFFIN_IMATE,
	    "iMate, ADB adapter",
	},
	{
	    USB_VENDOR_FREECOM, USB_PRODUCT_FREECOM_DVD,
	    "Connector for DVD drive",
	},
	{
	    USB_VENDOR_FTDI, USB_PRODUCT_FTDI_SERIAL,
	    "Serial converter",
	},
	{
	    USB_VENDOR_FUJIPHOTO, USB_PRODUCT_FUJIPHOTO_MASS0100,
	    "Mass Storage",
	},
	{
	    USB_VENDOR_HANDSPRING, USB_PRODUCT_HANDSPRING_VISOR,
	    "Handspring Visor",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_4100C,
	    "Scanjet 4100C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_S20,
	    "Photosmart S20",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_4200C,
	    "ScanJet 4200C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_6200C,
	    "ScanJet 6200C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_S20b,
	    "PhotoSmart S20",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_3300C,
	    "ScanJet 3300C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_5200C,
	    "Scanjet 5200C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_6300C,
	    "Scanjet 6300C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_5300C,
	    "Scanjet 5300C",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_970CSE,
	    "Deskjet 970Cse",
	},
	{
	    USB_VENDOR_HP, USB_PRODUCT_HP_P1100,
	    "Photosmart P1100",
	},
	{
	    USB_VENDOR_HP2, USB_PRODUCT_HP2_C500,
	    "PhotoSmart C500",
	},
	{
	    USB_VENDOR_IBM, USB_PRODUCT_IBM_USBCDROMDRIVE,
	    "USB CD-ROM Drive",
	},
	{
	    USB_VENDOR_INSIDEOUT, USB_PRODUCT_INSIDEOUT_EDGEPORT4,
	    "EdgePort/4 serial ports",
	},
	{
	    USB_VENDOR_INSYSTEM, USB_PRODUCT_INSYSTEM_F5U002,
	    "Parallel printer adapter",
	},
	{
	    USB_VENDOR_INSYSTEM, USB_PRODUCT_INSYSTEM_ISD110,
	    "IDE adapter ISD110",
	},
	{
	    USB_VENDOR_INSYSTEM, USB_PRODUCT_INSYSTEM_ISD105,
	    "IDE adapter ISD105",
	},
	{
	    USB_VENDOR_INSYSTEM, USB_PRODUCT_INSYSTEM_USBCABLE,
	    "USB cable",
	},
	{
	    USB_VENDOR_INTEL, USB_PRODUCT_INTEL_EASYPC_CAMERA,
	    "Easy PC Camera",
	},
	{
	    USB_VENDOR_INTEL, USB_PRODUCT_INTEL_TESTBOARD,
	    "82930 test board",
	},
	{
	    USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBETT,
	    "USB ETT",
	},
	{
	    USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBETTX,
	    "USB ETTX",
	},
	{
	    USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBETTXS,
	    "USB ETTX",
	},
	{
	    USB_VENDOR_IODATA, USB_PRODUCT_IODATA_USBRSAQ,
	    "RSAQ1 Serial Adapter",
	},
	{
	    USB_VENDOR_IOMEGA, USB_PRODUCT_IOMEGA_ZIP100,
	    "Zip 100",
	},
	{
	    USB_VENDOR_IOMEGA, USB_PRODUCT_IOMEGA_ZIP250,
	    "Zip 250",
	},
	{
	    USB_VENDOR_KLSI, USB_PRODUCT_KLSI_DUH3E10BT,
	    "USB Ethernet controller engine",
	},
	{
	    USB_VENDOR_KAWATSU, USB_PRODUCT_KAWATSU_MH4000P,
	    "MiniHub 4000P",
	},
	{
	    USB_VENDOR_KEISOKUGIKEN, USB_PRODUCT_KEISOKUGIKEN_USBDAQ,
	    "HKS-0200 USBDAQ",
	},
	{
	    USB_VENDOR_KLSI, USB_PRODUCT_KLSI_DUH3E10BT,
	    "10BT Ethernet",
	},
	{
	    USB_VENDOR_KENSINGTON, USB_PRODUCT_KENSINGTON_ORBIT,
	    "Orbit USB/PS2 trackball",
	},
	{
	    USB_VENDOR_KENSINGTON, USB_PRODUCT_KENSINGTON_TURBOBALL,
	    "TurboBall",
	},
	{
	    USB_VENDOR_KINGSTON, USB_PRODUCT_KINGSTON_KNU101TX,
	    "KNU101TX Ethernet",
	},
	{
	    USB_VENDOR_KODAK, USB_PRODUCT_KODAK_DC220,
	    "Digital Science DC220",
	},
	{
	    USB_VENDOR_KODAK, USB_PRODUCT_KODAK_DC260,
	    "Digital Science DC260",
	},
	{
	    USB_VENDOR_KODAK, USB_PRODUCT_KODAK_DC265,
	    "Digital Science DC265",
	},
	{
	    USB_VENDOR_KODAK, USB_PRODUCT_KODAK_DC290,
	    "Digital Science DC290",
	},
	{
	    USB_VENDOR_KODAK, USB_PRODUCT_KODAK_DC240,
	    "Digital Science DC240",
	},
	{
	    USB_VENDOR_KODAK, USB_PRODUCT_KODAK_DC280,
	    "Digital Science DC280",
	},
	{
	    USB_VENDOR_KONICA, USB_PRODUCT_KONICA_CAMERA,
	    "Digital Color Camera",
	},
	{
	    USB_VENDOR_KYE, USB_PRODUCT_KYE_NICHE,
	    "Niche mouse",
	},
	{
	    USB_VENDOR_KYE, USB_PRODUCT_KYE_NETSCROLL,
	    "Genius NetScroll mouse",
	},
	{
	    USB_VENDOR_KYE, USB_PRODUCT_KYE_FLIGHT2000,
	    "Flight 2000 joystick",
	},
	{
	    USB_VENDOR_KYE, USB_PRODUCT_KYE_VIVIDPRO,
	    "ColorPage Vivid-Pro scanner",
	},
	{
	    USB_VENDOR_LACIE, USB_PRODUCT_LACIE_CDRW,
	    "CD R/W",
	},
	{
	    USB_VENDOR_LEXMARK, USB_PRODUCT_LEXMARK_S2450,
	    "Optra S 2450",
	},
	{
	    USB_VENDOR_LINKSYS, USB_PRODUCT_LINKSYS_USB10T,
	    "USB10T Ethernet",
	},
	{
	    USB_VENDOR_LINKSYS, USB_PRODUCT_LINKSYS_USB100TX,
	    "USB100TX Ethernet",
	},
	{
	    USB_VENDOR_LINKSYS, USB_PRODUCT_LINKSYS_USB100H1,
	    "USB100H1 Ethernet/HPNA",
	},
	{
	    USB_VENDOR_LINKSYS, USB_PRODUCT_LINKSYS_USB10TA,
	    "USB10TA Ethernet",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_M2452,
	    "M2452",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_M4848,
	    "M4848",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_PAGESCAN,
	    "PageScan",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_QUICKCAMWEB,
	    "QuickCam Web",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_QUICKCAMPRO,
	    "QuickCam Pro",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_QUICKCAMEXP,
	    "QuickCam Express",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_QUICKCAM,
	    "QuickCam",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_N43,
	    "N43",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_N48,
	    "N48 mouse",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_MBA47,
	    "M-BA47 mouse",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_WMMOUSE,
	    "WingMan Gaming Mouse",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_BD58,
	    "BD58 mouse",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_BB13,
	    "USB-PS/2 Trackball",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_WMPAD,
	    "WingMan GamePad Extreme",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_WMJOY,
	    "WingMan Force joystick",
	},
	{
	    USB_VENDOR_LOGITECH, USB_PRODUCT_LOGITECH_QUICKCAMPRO2,
	    "QuickCam Pro",
	},
	{
	    USB_VENDOR_LUCENT, USB_PRODUCT_LUCENT_EVALKIT,
	    "USS-720 evaluation kit",
	},
	{
	    USB_VENDOR_MACALLY, USB_PRODUCT_MACALLY_MOUSE1,
	    "mouse",
	},
	{
	    USB_VENDOR_MCT, USB_PRODUCT_MCT_HUB0100,
	    "Hub",
	},
	{
	    USB_VENDOR_MCT, USB_PRODUCT_MCT_USB232,
	    "USB-232 Interface",
	},
	{
	    USB_VENDOR_MELCO, USB_PRODUCT_MELCO_LUATX1,
	    "LUA-TX Ethernet",
	},
	{
	    USB_VENDOR_MELCO, USB_PRODUCT_MELCO_LUATX5,
	    "LUA-TX Ethernet",
	},
	{
	    USB_VENDOR_METRICOM, USB_PRODUCT_METRICOM_RICOCHET_GS,
	    "Ricochet GS",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_SIDEPREC,
	    "SideWinder Precision Pro",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_INTELLIMOUSE,
	    "IntelliMouse",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_NATURALKBD,
	    "Natural",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_DDS80,
	    "Digital Sound System 80",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_SIDEWINDER,
	    "Sidewinder Precision Racing Wheel",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_INTELLIEYE,
	    "IntelliEye mouse",
	},
	{
	    USB_VENDOR_MICROSOFT, USB_PRODUCT_MICROSOFT_INETPRO,
	    "Internet Pro",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_336CX,
	    "Phantom 336CX - C3 scanner",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_X6U,
	    "ScanMaker X6 - X6U",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_C6,
	    "Phantom C6 scanner",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_336CX2,
	    "Phantom 336CX - C3 scanner",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_V6USL,
	    "ScanMaker V6USL",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_V6USL2,
	    "ScanMaker V6USL",
	},
	{
	    USB_VENDOR_MICROTEK, USB_PRODUCT_MICROTEK_V6UL,
	    "ScanMaker V6UL",
	},
	{
	    USB_VENDOR_MIDIMAN, USB_PRODUCT_MIDIMAN_MIDISPORT2X2,
	    "Midisport 2x2",
	},
	{
	    USB_VENDOR_MOTOROLA, USB_PRODUCT_MOTOROLA_MC141555,
	    "MC141555 hub controller",
	},
	{
	    USB_VENDOR_MULTITECH, USB_PRODUCT_MULTITECH_ATLAS,
	    "MT5634ZBA",
	},
	{
	    USB_VENDOR_MUSTEK, USB_PRODUCT_MUSTEK_1200CU,
	    "1200 CU scanner",
	},
	{
	    USB_VENDOR_MUSTEK, USB_PRODUCT_MUSTEK_600CU,
	    "600 CU scanner",
	},
	{
	    USB_VENDOR_MUSTEK, USB_PRODUCT_MUSTEK_1200USB,
	    "1200 scanner",
	},
	{
	    USB_VENDOR_MUSTEK, USB_PRODUCT_MUSTEK_1200UB,
	    "1200 UB scanner",
	},
	{
	    USB_VENDOR_MUSTEK, USB_PRODUCT_MUSTEK_MDC800,
	    "MDC-800 digital camera",
	},
	{
	    USB_VENDOR_NATIONAL, USB_PRODUCT_NATIONAL_BEARPAW,
	    "BearPaw 1200",
	},
	{
	    USB_VENDOR_NEC, USB_PRODUCT_NEC_HUB,
	    "hub",
	},
	{
	    USB_VENDOR_NEC, USB_PRODUCT_NEC_HUB_B,
	    "hub",
	},
	{
	    USB_VENDOR_NETCHIP, USB_PRODUCT_NETCHIP_TURBOCONNECT,
	    "Turbo-Connect",
	},
	{
	    USB_VENDOR_NETGEAR, USB_PRODUCT_NETGEAR_EA101,
	    "Ethernet",
	},
	{
	    USB_VENDOR_NIKON, USB_PRODUCT_NIKON_E990,
	    "Digital Camera E990",
	},
	{
	    USB_VENDOR_OMNIVISION, USB_PRODUCT_OMNIVISION_OV511,
	    "OV511 Camera",
	},
	{
	    USB_VENDOR_OMNIVISION, USB_PRODUCT_OMNIVISION_OV511PLUS,
	    "OV511+ Camera",
	},
	{
	    USB_VENDOR_PALM, USB_PRODUCT_PALM_SERIAL,
	    "USB Serial Adaptor",
	},
	{
	    USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_SERIAL1,
	    "Serial Converter",
	},
	{
	    USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET,
	    "Ethernet",
	},
	{
	    USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET3,
	    "At-Home Ethernet",
	},
	{
	    USB_VENDOR_PERACOM, USB_PRODUCT_PERACOM_ENET2,
	    "Ethernet",
	},
	{
	    USB_VENDOR_PHILIPS, USB_PRODUCT_PHILIPS_DSS350,
	    "DSS 350 Digital Speaker System",
	},
	{
	    USB_VENDOR_PHILIPS, USB_PRODUCT_PHILIPS_DSS,
	    "DSS XXX Digital Speaker System",
	},
	{
	    USB_VENDOR_PHILIPS, USB_PRODUCT_PHILIPS_HUB,
	    "hub",
	},
	{
	    USB_VENDOR_PHILIPS, USB_PRODUCT_PHILIPS_PCVC680K,
	    "PCVC680K Vesta Pro PC Camera",
	},
	{
	    USB_VENDOR_PHILIPS, USB_PRODUCT_PHILIPS_DSS150,
	    "DSS 150 Digital Speaker System",
	},
	{
	    USB_VENDOR_PHILIPSSEMI, USB_PRODUCT_PHILIPSSEMI_HUB1122,
	    "hub",
	},
	{
	    USB_VENDOR_PIENGINEERING, USB_PRODUCT_PIENGINEERING_PS2USB,
	    "PS2 to Mac Adapter",
	},
	{
	    USB_VENDOR_PLX, USB_PRODUCT_PLX_TESTBOARD,
	    "test board",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G2X300,
	    "G2-200 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G2E300,
	    "G2E-300 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G2300,
	    "G2-300 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G2E3002,
	    "G2E-300 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_9600,
	    "Colorado 9600 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_600U,
	    "Colorado 600u scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_19200,
	    "Colorado 19200 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_1200U,
	    "Colorado 1200u scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G600,
	    "G2-600 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_636I,
	    "ReadyScan 636i",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G2600,
	    "G2-600 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_G2E600,
	    "G2E-600 scanner",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_COMFORT,
	    "Comfort",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_MOUSEINABOX,
	    "Mouse-in-a-Box",
	},
	{
	    USB_VENDOR_PRIMAX, USB_PRODUCT_PRIMAX_PCGAUMS1,
	    "Sony PCGA-UMS1",
	},
	{
	    USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2301,
	    "PL2301 Host-Host interface",
	},
	{
	    USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2302,
	    "PL2302 Host-Host interface",
	},
	{
	    USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_RSAQ2,
	    "PL2303 Serial adapter (IODATA USB-RSAQ2)",
	},
	{
	    USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2303,
	    "PL2303 Serial adapter (ATEN/IOGEAR UC232A)",
	},
	{
	    USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_PL2305,
	    "Parallel printer adapter",
	},
	{
	    USB_VENDOR_PROLIFIC, USB_PRODUCT_PROLIFIC_ATAPI4,
	    "ATAPI-4 Bridge Controller",
	},
	{
	    USB_VENDOR_QTRONIX, USB_PRODUCT_QTRONIX_980N,
	    "Scorpion-980N",
	},
	{
	    USB_VENDOR_QUICKSHOT, USB_PRODUCT_QUICKSHOT_STRIKEPAD,
	    "USB StrikePad",
	},
	{
	    USB_VENDOR_RAINBOW, USB_PRODUCT_RAINBOW_IKEY2000,
	    "i-Key 2000",
	},
	{
	    USB_VENDOR_ROLAND, USB_PRODUCT_ROLAND_UM1,
	    "UM-1 MIDI I/F",
	},
	{
	    USB_VENDOR_ROCKFIRE, USB_PRODUCT_ROCKFIRE_GAMEPAD,
	    "gamepad 203USB",
	},
	{
	    USB_VENDOR_SANDISK, USB_PRODUCT_SANDISK_IMAGEMATE,
	    "USB ImageMate",
	},
	{
	    USB_VENDOR_SCANLOGIC, USB_PRODUCT_SCANLOGIC_336CX,
	    "Phantom 336CX - C3 scanner",
	},
	{
	    USB_VENDOR_SHUTTLE, USB_PRODUCT_SHUTTLE_EUSB,
	    "E-USB Bridge",
	},
	{
	    USB_VENDOR_SIIG, USB_PRODUCT_SIIG_DIGIFILMREADER,
	    "DigiFilm-Combo Reader",
	},
	{
	    USB_VENDOR_SILICONPORTALS, USB_PRODUCT_SILICONPORTALS_YAPPH_NF,
	    "YAP Phone (no firmware)",
	},
	{
	    USB_VENDOR_SILICONPORTALS, USB_PRODUCT_SILICONPORTALS_YAPPHONE,
	    "YAP Phone",
	},
	{
	    USB_VENDOR_SIRIUS, USB_PRODUCT_SIRIUS_ROADSTER,
	    "NetComm Roadster II 56 USB",
	},
	{
	    USB_VENDOR_SMARTBRIDGES, USB_PRODUCT_SMARTBRIDGES_SMARTLINK,
	    "SmartLink Ethernet",
	},
	{
	    USB_VENDOR_SMC, USB_PRODUCT_SMC_2102USB,
	    "10Mbps Ethernet",
	},
	{
	    USB_VENDOR_SMC, USB_PRODUCT_SMC_2202USB,
	    "10/100 Ethernet",
	},
	{
	    USB_VENDOR_SOHOWARE, USB_PRODUCT_SOHOWARE_NUB100,
	    "10/100 USB Ethernet",
	},
	{
	    USB_VENDOR_SOLIDYEAR, USB_PRODUCT_SOLIDYEAR_KEYBOARD,
	    "Keyboard",
	},
	{
	    USB_VENDOR_SONY, USB_PRODUCT_SONY_DSC,
	    "Sony DSC Cameras",
	},
	{
	    USB_VENDOR_SONY, USB_PRODUCT_SONY_MSC,
	    "Sony MSC Memorystick Slot",
	},
	{
	    USB_VENDOR_STMICRO, USB_PRODUCT_STMICRO_COMMUNICATOR,
	    "Communicator",
	},
	{
	    USB_VENDOR_STSN, USB_PRODUCT_STSN_STSN0001,
	    "Internet Access Device",
	},
	{
	    USB_VENDOR_SUN, USB_PRODUCT_SUN_KEYBOARD,
	    "Type 6 USB",
	},
	{
	    USB_VENDOR_DIAMOND2, USB_PRODUCT_DIAMOND2_SUPRAEXPRESS56K,
	    "Supra Express 56K",
	},
	{
	    USB_VENDOR_DIAMOND2, USB_PRODUCT_DIAMOND2_SUPRA2890,
	    "SupraMax 2890 56K",
	},
	{
	    USB_VENDOR_DIAMOND2, USB_PRODUCT_DIAMOND2_RIO600USB,
	    "Rio 600",
	},
	{
	    USB_VENDOR_TEAC, USB_PRODUCT_TEAC_FD05PUB,
	    "FD-05PUB",
	},
	{
	    USB_VENDOR_TELEX, USB_PRODUCT_TELEX_MIC1,
	    "Microphone",
	},
	{
	    USB_VENDOR_TI, USB_PRODUCT_TI_UTUSB41,
	    "UT-USB41 hub",
	},
	{
	    USB_VENDOR_TI, USB_PRODUCT_TI_TUSB2046,
	    "TUSB2046 hub",
	},
	{
	    USB_VENDOR_THRUST, USB_PRODUCT_THRUST_FUSION_PAD,
	    "Fusion Digital Gamepad",
	},
	{
	    USB_VENDOR_TODOS, USB_PRODUCT_TODOS_ARGOS_MINI,
	    "Argos Mini Smartcard Reader",
	},
	{
	    USB_VENDOR_TREK, USB_PRODUCT_TREK_THUMBDRIVE,
	    "ThumbDrive",
	},
	{
	    USB_VENDOR_UMAX, USB_PRODUCT_UMAX_ASTRA1236U,
	    "Astra 1236U Scanner",
	},
	{
	    USB_VENDOR_UMAX, USB_PRODUCT_UMAX_ASTRA1220U,
	    "Astra 1220U Scanner",
	},
	{
	    USB_VENDOR_UMAX, USB_PRODUCT_UMAX_ASTRA2000U,
	    "Astra 2000U Scanner",
	},
	{
	    USB_VENDOR_UMAX, USB_PRODUCT_UMAX_ASTRA2100U,
	    "Astra 2100U Scanner",
	},
	{
	    USB_VENDOR_UMAX, USB_PRODUCT_UMAX_ASTRA2200U,
	    "Astra 2200U Scanner",
	},
	{
	    USB_VENDOR_UNIACCESS, USB_PRODUCT_UNIACCESS_PANACHE,
	    "Panache Surf ISDN Adapter",
	},
	{
	    USB_VENDOR_VISION, USB_PRODUCT_VISION_VC6452V002,
	    "CPiA Camera",
	},
	{
	    USB_VENDOR_VISIONEER, USB_PRODUCT_VISIONEER_7600,
	    "OneTouch 7600",
	},
	{
	    USB_VENDOR_VISIONEER, USB_PRODUCT_VISIONEER_5300,
	    "OneTouch 5300",
	},
	{
	    USB_VENDOR_VISIONEER, USB_PRODUCT_VISIONEER_6100,
	    "OneTouch 6100",
	},
	{
	    USB_VENDOR_VISIONEER, USB_PRODUCT_VISIONEER_6200,
	    "OneTouch 6200",
	},
	{
	    USB_VENDOR_VISIONEER, USB_PRODUCT_VISIONEER_8100,
	    "OneTouch 8100",
	},
	{
	    USB_VENDOR_VISIONEER, USB_PRODUCT_VISIONEER_8600,
	    "OneTouch 8600",
	},
	{
	    USB_VENDOR_WACOM, USB_PRODUCT_WACOM_CT0405U,
	    "CT-0405-U Tablet",
	},
	{
	    USB_VENDOR_WACOM, USB_PRODUCT_WACOM_GRAPHIRE,
	    "Graphire",
	},
	{
	    USB_VENDOR_WACOM, USB_PRODUCT_WACOM_INTUOSA5,
	    "Intuos A5",
	},
	{
	    USB_VENDOR_YEDATA, USB_PRODUCT_YEDATA_FLASHBUSTERU,
	    "Flashbuster-U",
	},
	{
	    USB_VENDOR_YANO, USB_PRODUCT_YANO_U640MO,
	    "U640MO-03",
	},
	{
	    USB_VENDOR_YAMAHA, USB_PRODUCT_YAMAHA_UX256,
	    "UX256 MIDI I/F",
	},
	{
	    USB_VENDOR_ZOOM, USB_PRODUCT_ZOOM_2986L,
	    "2986L",
	},
	{
	    USB_VENDOR_ZYXEL, USB_PRODUCT_ZYXEL_980N,
	    "Scorpion-980N",
	},
	{ 0, 0, NULL }
};

const struct usb_known_vendor usb_known_vendors[] = {
	{
	    USB_VENDOR_AOX,
	    "AOX",
	},
	{
	    USB_VENDOR_ATMEL,
	    "Atmel",
	},
	{
	    USB_VENDOR_HP,
	    "Hewlett Packard",
	},
	{
	    USB_VENDOR_ADAPTEC,
	    "Adaptec",
	},
	{
	    USB_VENDOR_NATIONAL,
	    "National Semiconductor",
	},
	{
	    USB_VENDOR_ACERLABS,
	    "Acer Labs",
	},
	{
	    USB_VENDOR_FTDI,
	    "Future Technology Devices",
	},
	{
	    USB_VENDOR_NEC,
	    "NEC",
	},
	{
	    USB_VENDOR_KODAK,
	    "Eastman Kodak",
	},
	{
	    USB_VENDOR_MELCO,
	    "Melco",
	},
	{
	    USB_VENDOR_CREATIVELABS,
	    "Creative Labs",
	},
	{
	    USB_VENDOR_ADI,
	    "ADI Systems",
	},
	{
	    USB_VENDOR_CATC,
	    "Computer Access Technology",
	},
	{
	    USB_VENDOR_GRAVIS,
	    "Advanced Gravis Computer",
	},
	{
	    USB_VENDOR_SUN,
	    "Sun Microsystems",
	},
	{
	    USB_VENDOR_AMD,
	    "Advanced Micro Devices",
	},
	{
	    USB_VENDOR_LEXMARK,
	    "Lexmark International",
	},
	{
	    USB_VENDOR_NANAO,
	    "NANAO",
	},
	{
	    USB_VENDOR_ALPS,
	    "Alps Electric",
	},
	{
	    USB_VENDOR_THRUST,
	    "Thrustmaster",
	},
	{
	    USB_VENDOR_TI,
	    "Texas Instruments",
	},
	{
	    USB_VENDOR_ANALOGDEVICES,
	    "Analog Devices",
	},
	{
	    USB_VENDOR_KYE,
	    "KYE Systems",
	},
	{
	    USB_VENDOR_DIAMOND2,
	    "Diamond (Supra)",
	},
	{
	    USB_VENDOR_MICROSOFT,
	    "Microsoft",
	},
	{
	    USB_VENDOR_PRIMAX,
	    "Primax Electronics",
	},
	{
	    USB_VENDOR_AMP,
	    "AMP",
	},
	{
	    USB_VENDOR_CHERRY,
	    "Cherry Mikroschalter",
	},
	{
	    USB_VENDOR_MEGATRENDS,
	    "American Megatrends",
	},
	{
	    USB_VENDOR_LOGITECH,
	    "Logitech",
	},
	{
	    USB_VENDOR_BTC,
	    "Behavior Tech. Computer",
	},
	{
	    USB_VENDOR_PHILIPS,
	    "Philips",
	},
	{
	    USB_VENDOR_CONNECTIX,
	    "Connectix",
	},
	{
	    USB_VENDOR_KENSINGTON,
	    "Kensington",
	},
	{
	    USB_VENDOR_LUCENT,
	    "Lucent",
	},
	{
	    USB_VENDOR_STMICRO,
	    "STMicroelectronics",
	},
	{
	    USB_VENDOR_YAMAHA,
	    "YAMAHA",
	},
	{
	    USB_VENDOR_COMPAQ,
	    "Compaq",
	},
	{
	    USB_VENDOR_ACERP,
	    "Acer Peripherals",
	},
	{
	    USB_VENDOR_VISIONEER,
	    "Visioneer",
	},
	{
	    USB_VENDOR_CANON,
	    "Canon",
	},
	{
	    USB_VENDOR_NIKON,
	    "Nikon",
	},
	{
	    USB_VENDOR_IBM,
	    "IBM",
	},
	{
	    USB_VENDOR_CYPRESS,
	    "Cypress Semiconductor",
	},
	{
	    USB_VENDOR_EPSON,
	    "Seiko Epson",
	},
	{
	    USB_VENDOR_RAINBOW,
	    "Rainbow Technologies",
	},
	{
	    USB_VENDOR_IODATA,
	    "I/O Data",
	},
	{
	    USB_VENDOR_3COMUSR,
	    "U.S. Robotics",
	},
	{
	    USB_VENDOR_METHODE,
	    "Methode Electronics Far East",
	},
	{
	    USB_VENDOR_MAXISWITCH,
	    "Maxi Switch",
	},
	{
	    USB_VENDOR_LOCKHEEDMER,
	    "Lockheed Martin Energy Research",
	},
	{
	    USB_VENDOR_FUJITSU,
	    "Fujitsu",
	},
	{
	    USB_VENDOR_TOSHIBAAM,
	    "Toshiba America Electronic Components",
	},
	{
	    USB_VENDOR_MICROMACRO,
	    "Micro Macro Technologies",
	},
	{
	    USB_VENDOR_KONICA,
	    "Konica",
	},
	{
	    USB_VENDOR_LITEON,
	    "Lite-On Technology",
	},
	{
	    USB_VENDOR_FUJIPHOTO,
	    "Fuji Photo Film",
	},
	{
	    USB_VENDOR_PHILIPSSEMI,
	    "Philips Semiconductors",
	},
	{
	    USB_VENDOR_TATUNG,
	    "Tatung Co. Of America",
	},
	{
	    USB_VENDOR_SCANLOGIC,
	    "ScanLogic",
	},
	{
	    USB_VENDOR_MYSON,
	    "Myson Technology",
	},
	{
	    USB_VENDOR_DIGI2,
	    "Digi",
	},
	{
	    USB_VENDOR_ITTCANON,
	    "ITT Canon",
	},
	{
	    USB_VENDOR_ALTEC,
	    "Altec Lansing",
	},
	{
	    USB_VENDOR_SHUTTLE,
	    "Shuttle Technology",
	},
	{
	    USB_VENDOR_ANNABOOKS,
	    "Annabooks",
	},
	{
	    USB_VENDOR_CHICONY,
	    "Chicony Electronics",
	},
	{
	    USB_VENDOR_BROTHER,
	    "Brother Industries",
	},
	{
	    USB_VENDOR_DALLAS,
	    "Dallas Semiconductor",
	},
	{
	    USB_VENDOR_ACER,
	    "Acer",
	},
	{
	    USB_VENDOR_3COM,
	    "3Com",
	},
	{
	    USB_VENDOR_AZTECH,
	    "Aztech Systems",
	},
	{
	    USB_VENDOR_BELKIN,
	    "Belkin Components",
	},
	{
	    USB_VENDOR_KAWATSU,
	    "Kawatsu Semiconductor",
	},
	{
	    USB_VENDOR_APC,
	    "American Power Conversion",
	},
	{
	    USB_VENDOR_CONNECTEK,
	    "Advanced Connectek USA",
	},
	{
	    USB_VENDOR_NETCHIP,
	    "NetChip Technology",
	},
	{
	    USB_VENDOR_ALTRA,
	    "ALTRA",
	},
	{
	    USB_VENDOR_ATI,
	    "ATI Technologies",
	},
	{
	    USB_VENDOR_AKS,
	    "Aladdin Knowledge Systems",
	},
	{
	    USB_VENDOR_UNIACCESS,
	    "Universal Access",
	},
	{
	    USB_VENDOR_ANCHOR,
	    "Anchor Chips",
	},
	{
	    USB_VENDOR_SONY,
	    "Sony",
	},
	{
	    USB_VENDOR_VISION,
	    "VLSI Vision",
	},
	{
	    USB_VENDOR_ASAHIKASEI,
	    "Asahi Kasei Microsystems",
	},
	{
	    USB_VENDOR_ATEN,
	    "ATEN International",
	},
	{
	    USB_VENDOR_MUSTEK,
	    "Mustek Systems",
	},
	{
	    USB_VENDOR_TELEX,
	    "Telex Communications",
	},
	{
	    USB_VENDOR_PERACOM,
	    "Peracom Networks",
	},
	{
	    USB_VENDOR_ALCOR2,
	    "Alcor Micro",
	},
	{
	    USB_VENDOR_WACOM,
	    "WACOM",
	},
	{
	    USB_VENDOR_ETEK,
	    "e-TEK Labs",
	},
	{
	    USB_VENDOR_EIZO,
	    "EIZO",
	},
	{
	    USB_VENDOR_ELECOM,
	    "Elecom",
	},
	{
	    USB_VENDOR_BAFO,
	    "BAFO/Quality Computer Accessories",
	},
	{
	    USB_VENDOR_YEDATA,
	    "Y-E Data",
	},
	{
	    USB_VENDOR_AVM,
	    "AVM",
	},
	{
	    USB_VENDOR_QUICKSHOT,
	    "Quickshot",
	},
	{
	    USB_VENDOR_ROLAND,
	    "Roland",
	},
	{
	    USB_VENDOR_ROCKFIRE,
	    "Rockfire",
	},
	{
	    USB_VENDOR_ZYXEL,
	    "ZyXEL Communication",
	},
	{
	    USB_VENDOR_ALCOR,
	    "Alcor Micro",
	},
	{
	    USB_VENDOR_IOMEGA,
	    "Iomega",
	},
	{
	    USB_VENDOR_ATREND,
	    "A-Trend Technology",
	},
	{
	    USB_VENDOR_AID,
	    "Advanced Input Devices",
	},
	{
	    USB_VENDOR_LACIE,
	    "LaCie",
	},
	{
	    USB_VENDOR_OMNIVISION,
	    "OmniVision",
	},
	{
	    USB_VENDOR_INSYSTEM,
	    "In-System Design",
	},
	{
	    USB_VENDOR_APPLE,
	    "Apple Computer",
	},
	{
	    USB_VENDOR_DIGI,
	    "Digi International",
	},
	{
	    USB_VENDOR_QTRONIX,
	    "Qtronix",
	},
	{
	    USB_VENDOR_ELSA,
	    "ELSA",
	},
	{
	    USB_VENDOR_BRAINBOXES,
	    "Brainboxes Limited",
	},
	{
	    USB_VENDOR_AXIOHM,
	    "Axiohm Transaction Solutions",
	},
	{
	    USB_VENDOR_MICROTEK,
	    "Microtek",
	},
	{
	    USB_VENDOR_SYMBOL,
	    "Symbol Technologies",
	},
	{
	    USB_VENDOR_GENESYS,
	    "Genesys Logic",
	},
	{
	    USB_VENDOR_FUJI,
	    "Fuji Electric",
	},
	{
	    USB_VENDOR_KEITHLEY,
	    "Keithley Instruments",
	},
	{
	    USB_VENDOR_EIZONANAO,
	    "EIZO Nanao",
	},
	{
	    USB_VENDOR_KLSI,
	    "Kawasaki LSI",
	},
	{
	    USB_VENDOR_FFC,
	    "FFC Limited",
	},
	{
	    USB_VENDOR_ANKO,
	    "Anko Electronic",
	},
	{
	    USB_VENDOR_PIENGINEERING,
	    "P.I. Engineering",
	},
	{
	    USB_VENDOR_AOC,
	    "AOC International",
	},
	{
	    USB_VENDOR_CHIC,
	    "Chic Technology",
	},
	{
	    USB_VENDOR_BARCO,
	    "Barco Display Systems",
	},
	{
	    USB_VENDOR_BRIDGE,
	    "Bridge Information",
	},
	{
	    USB_VENDOR_SOLIDYEAR,
	    "Solid Year",
	},
	{
	    USB_VENDOR_BIORAD,
	    "Bio-Rad Laboratories",
	},
	{
	    USB_VENDOR_MACALLY,
	    "Macally",
	},
	{
	    USB_VENDOR_ACTLABS,
	    "Act Labs",
	},
	{
	    USB_VENDOR_ALARIS,
	    "Alaris",
	},
	{
	    USB_VENDOR_APEX,
	    "Apex",
	},
	{
	    USB_VENDOR_AVISION,
	    "Avision",
	},
	{
	    USB_VENDOR_TEAC,
	    "TEAC",
	},
	{
	    USB_VENDOR_LINKSYS,
	    "Linksys",
	},
	{
	    USB_VENDOR_ACERSA,
	    "Acer Semiconductor America",
	},
	{
	    USB_VENDOR_AIWA,
	    "Aiwa",
	},
	{
	    USB_VENDOR_ACARD,
	    "ACARD Technology",
	},
	{
	    USB_VENDOR_PROLIFIC,
	    "Prolific Technology",
	},
	{
	    USB_VENDOR_ADVANCELOGIC,
	    "Avance Logic",
	},
	{
	    USB_VENDOR_CTX,
	    "Chuntex",
	},
	{
	    USB_VENDOR_ASKEY,
	    "Askey Computer",
	},
	{
	    USB_VENDOR_ALCATELT,
	    "Alcatel Telecom",
	},
	{
	    USB_VENDOR_AGFA,
	    "AGFA-Gevaert NV",
	},
	{
	    USB_VENDOR_ASIAMD,
	    "Asia Microelectronic Development",
	},
	{
	    USB_VENDOR_BIZLINK,
	    "Bizlink International",
	},
	{
	    USB_VENDOR_AASHIMA,
	    "Aashima Technology B.V.",
	},
	{
	    USB_VENDOR_MULTITECH,
	    "MultiTech",
	},
	{
	    USB_VENDOR_ADS,
	    "ADS Technologies",
	},
	{
	    USB_VENDOR_ALCATELM,
	    "Alcatel Microelectronics",
	},
	{
	    USB_VENDOR_SIRIUS,
	    "Sirius Technologies",
	},
	{
	    USB_VENDOR_BOSTON,
	    "Boston Acoustics",
	},
	{
	    USB_VENDOR_SMC,
	    "Standard Microsystems",
	},
	{
	    USB_VENDOR_MCT,
	    "MCT",
	},
	{
	    USB_VENDOR_AUREAL,
	    "Aureal Semiconductor",
	},
	{
	    USB_VENDOR_MIDIMAN,
	    "Midiman",
	},
	{
	    USB_VENDOR_GRIFFIN,
	    "Griffin Technology",
	},
	{
	    USB_VENDOR_SANDISK,
	    "SanDisk",
	},
	{
	    USB_VENDOR_BRIMAX,
	    "Brimax",
	},
	{
	    USB_VENDOR_AXIS,
	    "Axis Communications AB",
	},
	{
	    USB_VENDOR_ABL,
	    "ABL Electronics",
	},
	{
	    USB_VENDOR_ALFADATA,
	    "Alfadata Computer",
	},
	{
	    USB_VENDOR_NATIONALTECH,
	    "National Technical Systems",
	},
	{
	    USB_VENDOR_ONNTO,
	    "Onnto",
	},
	{
	    USB_VENDOR_BE,
	    "Be",
	},
	{
	    USB_VENDOR_ADMTEK,
	    "ADMtek",
	},
	{
	    USB_VENDOR_COREGA,
	    "Corega",
	},
	{
	    USB_VENDOR_FREECOM,
	    "Freecom",
	},
	{
	    USB_VENDOR_MICROTECH,
	    "Microtech",
	},
	{
	    USB_VENDOR_ABOCOM,
	    "AboCom Systems",
	},
	{
	    USB_VENDOR_KEISOKUGIKEN,
	    "Keisokugiken",
	},
	{
	    USB_VENDOR_APG,
	    "APG Cash Drawer",
	},
	{
	    USB_VENDOR_BUG,
	    "B.U.G.",
	},
	{
	    USB_VENDOR_ALLIEDTELESYN,
	    "Allied Telesyn International",
	},
	{
	    USB_VENDOR_AVERMEDIA,
	    "AVerMedia Technologies",
	},
	{
	    USB_VENDOR_SIIG,
	    "SIIG",
	},
	{
	    USB_VENDOR_APTIO,
	    "Aptio Products",
	},
	{
	    USB_VENDOR_ARASAN,
	    "Arasan Chip Systems",
	},
	{
	    USB_VENDOR_ALLIEDCABLE,
	    "Allied Cable",
	},
	{
	    USB_VENDOR_STSN,
	    "STSN",
	},
	{
	    USB_VENDOR_ZOOM,
	    "Zoom Telephonics",
	},
	{
	    USB_VENDOR_BROADLOGIC,
	    "BroadLogic",
	},
	{
	    USB_VENDOR_HANDSPRING,
	    "Handspring",
	},
	{
	    USB_VENDOR_ACTIONSTAR,
	    "Action Star Enterprise",
	},
	{
	    USB_VENDOR_PALM,
	    "Palm Computing",
	},
	{
	    USB_VENDOR_ACCTON,
	    "Accton Technology",
	},
	{
	    USB_VENDOR_DIAMOND,
	    "Diamond",
	},
	{
	    USB_VENDOR_NETGEAR,
	    "BayNETGEAR",
	},
	{
	    USB_VENDOR_ACTIVEWIRE,
	    "ActiveWire",
	},
	{
	    USB_VENDOR_METRICOM,
	    "Metricom",
	},
	{
	    USB_VENDOR_ADESSOKBTEK,
	    "ADESSO/Kbtek America",
	},
	{
	    USB_VENDOR_APT,
	    "APT Technologies",
	},
	{
	    USB_VENDOR_BOCARESEARCH,
	    "Boca Research",
	},
	{
	    USB_VENDOR_ANDREA,
	    "Andrea Electronics",
	},
	{
	    USB_VENDOR_BURRBROWN,
	    "Burr-Brown Japan",
	},
	{
	    USB_VENDOR_2WIRE,
	    "2Wire, Inc",
	},
	{
	    USB_VENDOR_AIPTEK,
	    "AIPTEK International",
	},
	{
	    USB_VENDOR_SMARTBRIDGES,
	    "SmartBridges",
	},
	{
	    USB_VENDOR_BILLIONTON,
	    "Billionton Systems",
	},
	{
	    USB_VENDOR_EXTENDED,
	    "Extended Systems",
	},
	{
	    USB_VENDOR_AUTHENTEC,
	    "AuthenTec",
	},
	{
	    USB_VENDOR_ALATION,
	    "Alation Systems",
	},
	{
	    USB_VENDOR_BIOMETRIC,
	    "American Biometric Company",
	},
	{
	    USB_VENDOR_YANO,
	    "Yano",
	},
	{
	    USB_VENDOR_KINGSTON,
	    "Kingston Technology",
	},
	{
	    USB_VENDOR_BLUEWATER,
	    "BlueWater Systems",
	},
	{
	    USB_VENDOR_AGILENT,
	    "Agilent Technologies",
	},
	{
	    USB_VENDOR_ADIRONDACK,
	    "Adirondack Wire & Cable",
	},
	{
	    USB_VENDOR_BECKHOFF,
	    "Beckhoff",
	},
	{
	    USB_VENDOR_ALTIUS,
	    "Altius Solutions",
	},
	{
	    USB_VENDOR_ARRIS,
	    "Arris Interactive",
	},
	{
	    USB_VENDOR_ACTIVCARD,
	    "ACTIVCARD",
	},
	{
	    USB_VENDOR_ACTISYS,
	    "ACTiSYS",
	},
	{
	    USB_VENDOR_AFOURTECH,
	    "A-FOUR TECH",
	},
	{
	    USB_VENDOR_AIMEX,
	    "AIMEX",
	},
	{
	    USB_VENDOR_ADDONICS,
	    "Addonics Technologies",
	},
	{
	    USB_VENDOR_AKAI,
	    "AKAI professional M.I.",
	},
	{
	    USB_VENDOR_ARESCOM,
	    "ARESCOM",
	},
	{
	    USB_VENDOR_BAY,
	    "Bay Associates",
	},
	{
	    USB_VENDOR_ALTERA,
	    "Altera",
	},
	{
	    USB_VENDOR_TREK,
	    "Trek Technology",
	},
	{
	    USB_VENDOR_ASAHIOPTICAL,
	    "Asahi Optical",
	},
	{
	    USB_VENDOR_BOCASYSTEMS,
	    "Boca Systems",
	},
	{
	    USB_VENDOR_BROADCOM,
	    "Broadcom",
	},
	{
	    USB_VENDOR_GEOCAST,
	    "Geocast Network Systems",
	},
	{
	    USB_VENDOR_TODOS,
	    "Todos Data System",
	},
	{
	    USB_VENDOR_MOTOROLA,
	    "Motorola",
	},
	{
	    USB_VENDOR_PLX,
	    "PLX",
	},
	{
	    USB_VENDOR_BELKIN2,
	    "Belkin Components",
	},
	{
	    USB_VENDOR_SILICONPORTALS,
	    "Silicon Portals",
	},
	{
	    USB_VENDOR_SOHOWARE,
	    "SOHOware",
	},
	{
	    USB_VENDOR_UMAX,
	    "UMAX Data Systems",
	},
	{
	    USB_VENDOR_INSIDEOUT,
	    "Inside Out Networks",
	},
	{
	    USB_VENDOR_ENTREGA,
	    "Entrega",
	},
	{
	    USB_VENDOR_ACTIONTEC,
	    "Actiontec Electronics",
	},
	{
	    USB_VENDOR_DLINK,
	    "D-Link",
	},
	{
	    USB_VENDOR_INTEL,
	    "Intel",
	},
	{
	    USB_VENDOR_HP2,
	    "Hewlett Packard",
	},
	{ 0, NULL }
};
