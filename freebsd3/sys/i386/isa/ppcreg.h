/*-
 * Copyright (c) 1997 Nicolas Souchu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: ppcreg.h,v 1.7 1999/01/10 16:41:13 nsouch Exp $
 *
 */
#ifndef __PPCREG_H
#define __PPCREG_H

/*
 * Parallel Port Chipset type.
 */
#define SMC_LIKE	0
#define SMC_37C665GT	1
#define SMC_37C666GT	2
#define NS_PC87332	3
#define NS_PC87306	4
#define INTEL_820191AA	5	/* XXX not implemented */
#define GENERIC		6
#define WINB_W83877F	7
#define WINB_W83877AF	8
#define WINB_UNKNOWN	9
#define NS_PC87334	10

/*
 * Generic structure to hold parallel port chipset info.
 */
struct ppc_data {

	int ppc_unit;
	int ppc_type;

	int ppc_mode;		/* chipset current mode */
	int ppc_avm;		/* chipset available modes */

#define PPC_IRQ_NONE		0x0
#define PPC_IRQ_nACK		0x1
#define PPC_IRQ_DMA		0x2
#define PPC_IRQ_FIFO		0x4
#define PPC_IRQ_nFAULT		0x8
	int ppc_irqstat;	/* remind irq settings */

#define PPC_DMA_INIT		0x01
#define PPC_DMA_STARTED		0x02
#define PPC_DMA_COMPLETE	0x03
#define PPC_DMA_INTERRUPTED	0x04
#define PPC_DMA_ERROR		0x05
	int ppc_dmastat;	/* dma state */
	int ppc_dmachan;	/* dma channel */
	int ppc_dmaflags;	/* dma transfer flags */
	caddr_t ppc_dmaddr;	/* buffer address */
	u_int ppc_dmacnt;	/* count of bytes sent with dma */

#define PPC_PWORD_MASK	0x30
#define PPC_PWORD_16	0x00
#define PPC_PWORD_8	0x10
#define PPC_PWORD_32	0x20
	char ppc_pword;		/* PWord size */
	short ppc_fifo;		/* FIFO threshold */

	short ppc_wthr;		/* writeIntrThresold */
	short ppc_rthr;		/* readIntrThresold */

#define ppc_base ppc_link.base
#define ppc_epp ppc_link.epp_protocol
#define ppc_irq ppc_link.id_irq
#define ppc_subm ppc_link.submicroseq
#define ppc_ptr ppc_link.ptr
#define ppc_accum ppc_link.accum

	unsigned char ppc_flags;

	struct ppb_link ppc_link;
};

/*
 * Parallel Port Chipset registers.
 */
#define PPC_SPP_DTR	0	/* SPP data register */
#define PPC_ECP_A_FIFO	0	/* ECP Address fifo register */
#define PPC_SPP_STR	1	/* SPP status register */
#define PPC_SPP_CTR	2	/* SPP control register */
#define PPC_EPP_DATA	4	/* EPP data register (8, 16 or 32 bit) */
#define PPC_ECP_D_FIFO	0x400	/* ECP Data fifo register */
#define PPC_ECP_CNFGA	0x400	/* Configuration register A */
#define PPC_ECP_CNFGB	0x401	/* Configuration register B */
#define PPC_ECP_ECR	0x402	/* ECP extended control register */

#define PPC_FIFO_EMPTY	0x1	/* ecr register - bit 0 */
#define PPC_FIFO_FULL	0x2	/* ecr register - bit 1 */
#define PPC_SERVICE_INTR 0x4	/* ecr register - bit 2 */
#define PPC_ENABLE_DMA	0x8	/* ecr register - bit 3 */
#define PPC_nFAULT_INTR	0x10	/* ecr register - bit 4 */
#define PPC_ECR_STD	0x0
#define PPC_ECR_PS2	0x20
#define PPC_ECR_FIFO	0x40
#define PPC_ECR_ECP	0x60
#define PPC_ECR_EPP	0x80

#define PPC_DISABLE_INTR	(PPC_SERVICE_INTR | PPC_nFAULT_INTR)
#define PPC_ECR_RESET		(PPC_ECR_PS2 | PPC_DISABLE_INTR)

#define r_dtr(ppc) (inb((ppc)->ppc_base + PPC_SPP_DTR))
#define r_str(ppc) (inb((ppc)->ppc_base + PPC_SPP_STR))
#define r_ctr(ppc) (inb((ppc)->ppc_base + PPC_SPP_CTR))
#define r_epp(ppc) (inb((ppc)->ppc_base + PPC_EPP_DATA))
#define r_cnfgA(ppc) (inb((ppc)->ppc_base + PPC_ECP_CNFGA))
#define r_cnfgB(ppc) (inb((ppc)->ppc_base + PPC_ECP_CNFGB))
#define r_ecr(ppc) (inb((ppc)->ppc_base + PPC_ECP_ECR))
#define r_fifo(ppc) (inb((ppc)->ppc_base + PPC_ECP_D_FIFO))

#define w_dtr(ppc,byte) outb((ppc)->ppc_base + PPC_SPP_DTR, byte)
#define w_str(ppc,byte) outb((ppc)->ppc_base + PPC_SPP_STR, byte)
#define w_ctr(ppc,byte) outb((ppc)->ppc_base + PPC_SPP_CTR, byte)
#define w_epp(ppc,byte) outb((ppc)->ppc_base + PPC_EPP_DATA, byte)
#define w_ecr(ppc,byte) outb((ppc)->ppc_base + PPC_ECP_ECR, byte)
#define w_fifo(ppc,byte) outb((ppc)->ppc_base + PPC_ECP_D_FIFO, byte)

/*
 * Register defines for the PC873xx parts
 */

#define PC873_FER	0x00
#define PC873_PPENABLE	(1<<0)
#define PC873_FAR	0x01
#define PC873_PTR	0x02
#define PC873_CFGLOCK	(1<<6)
#define PC873_EPPRDIR	(1<<7)
#define PC873_EXTENDED	(1<<7)
#define PC873_LPTBIRQ7	(1<<3)
#define PC873_FCR	0x03
#define PC873_ZWS	(1<<5)
#define PC873_ZWSPWDN	(1<<6)
#define PC873_PCR	0x04
#define PC873_EPPEN	(1<<0)
#define PC873_EPP19	(1<<1)
#define PC873_ECPEN	(1<<2)
#define PC873_ECPCLK	(1<<3)
#define PC873_PMC	0x06
#define PC873_TUP	0x07
#define PC873_SID	0x08
#define PC873_PNP0	0x1b
#define PC873_PNP1	0x1c
#define PC873_LPTBA	0x19

/*
 * Register defines for the SMC FDC37C66xGT parts
 */

/* Init codes */
#define SMC665_iCODE	0x55
#define SMC666_iCODE	0x44

/* Base configuration ports */
#define SMC66x_CSR	0x3F0
#define SMC666_CSR	0x370		/* hard-configured value for 666 */

/* Bits */
#define SMC_CR1_ADDR	0x3		/* bit 0 and 1 */
#define SMC_CR1_MODE	(1<<3)		/* bit 3 */
#define SMC_CR4_EMODE	0x3		/* bits 0 and 1 */
#define SMC_CR4_EPPTYPE	(1<<6)		/* bit 6 */

/* Extended modes */
#define SMC_SPP		0x0		/* SPP */
#define SMC_EPPSPP	0x1		/* EPP and SPP */
#define SMC_ECP		0x2 		/* ECP */
#define SMC_ECPEPP	0x3		/* ECP and EPP */

/*
 * Register defines for the Winbond W83877F parts
 */

#define WINB_W83877F_ID		0xa
#define WINB_W83877AF_ID	0xb

/* Configuration bits */
#define WINB_HEFERE	(1<<5)		/* CROC bit 5 */
#define WINB_HEFRAS	(1<<0)		/* CR16 bit 0 */

#define WINB_PNPCVS	(1<<2)		/* CR16 bit 2 */
#define WINB_CHIPID	0xf		/* CR9 bits 0-3 */

#define WINB_PRTMODS0	(1<<2)		/* CR0 bit 2 */
#define WINB_PRTMODS1	(1<<3)		/* CR0 bit 3 */
#define WINB_PRTMODS2	(1<<7)		/* CR9 bit 7 */

/* W83877F modes: CR9/bit7 | CR0/bit3 | CR0/bit2 */
#define WINB_W83757	0x0
#define WINB_EXTFDC	0x4
#define WINB_EXTADP	0x8
#define WINB_EXT2FDD	0xc
#define WINB_JOYSTICK	0x80

#define WINB_PARALLEL	0x80
#define WINB_EPP_SPP	0x4
#define WINB_ECP	0x8
#define WINB_ECP_EPP	0xc

#endif
