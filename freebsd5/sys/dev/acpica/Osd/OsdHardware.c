/*-
 * Copyright (c) 2000, 2001 Michael Smith
 * Copyright (c) 2000 BSDi
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
 *	$FreeBSD: src/sys/dev/acpica/Osd/OsdHardware.c,v 1.5 2002/03/19 11:01:52 peter Exp $
 */

/*
 * 6.7 : Hardware Abstraction
 */

#include "acpi.h"

#include <machine/bus_pio.h>
#include <machine/bus.h>
#include <machine/pci_cfgreg.h>

/*
 * ACPICA's rather gung-ho approach to hardware resource ownership is a little
 * troublesome insofar as there is no easy way for us to know in advance 
 * exactly which I/O resources it's going to want to use.
 * 
 * In order to deal with this, we ignore resource ownership entirely, and simply
 * use the native I/O space accessor functionality.  This is Evil, but it works.
 *
 * XXX use an intermediate #define for the tag/handle
 */

#ifdef __i386__
#define ACPI_BUS_SPACE_IO	I386_BUS_SPACE_IO
#define ACPI_BUS_HANDLE		0
#endif
#ifdef __ia64__
#define ACPI_BUS_SPACE_IO	IA64_BUS_SPACE_IO
#define ACPI_BUS_HANDLE		0
#endif

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS	InPort,
    void		*Value,
    UINT32		Width)
{
    switch (Width) {
    case 8:
        *(u_int8_t *)Value = bus_space_read_1(ACPI_BUS_SPACE_IO, ACPI_BUS_HANDLE, InPort);
        break;
    case 16:
        *(u_int16_t *)Value = bus_space_read_2(ACPI_BUS_SPACE_IO, ACPI_BUS_HANDLE, InPort);
        break;
    case 32:
        *(u_int32_t *)Value = bus_space_read_4(ACPI_BUS_SPACE_IO, ACPI_BUS_HANDLE, InPort);
        break;
    default:
        /* debug trap goes here */
	break;
    }

    return(AE_OK);
}

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS	OutPort,
    ACPI_INTEGER	Value,
    UINT32		Width)
{
    switch (Width) {
    case 8:
        bus_space_write_1(ACPI_BUS_SPACE_IO, ACPI_BUS_HANDLE, OutPort, Value);
        break;
    case 16:
        bus_space_write_2(ACPI_BUS_SPACE_IO, ACPI_BUS_HANDLE, OutPort, Value);
        break;
    case 32:
        bus_space_write_4(ACPI_BUS_SPACE_IO, ACPI_BUS_HANDLE, OutPort, Value);
        break;
    default:
        /* debug trap goes here */
	break;
    }

    return(AE_OK);
}

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID		*PciId,
    UINT32		Register,
    void		*Value,
    UINT32		Width)
{
    u_int32_t	byte_width = Width / 8;
    u_int32_t	val;

    if (!pci_cfgregopen())
        return(AE_NOT_EXIST);

    val = pci_cfgregread(PciId->Bus, PciId->Device, PciId->Function, Register, byte_width);
    switch (Width) {
    case 8:
	*(u_int8_t *)Value = val & 0xff;
	break;
    case 16:
	*(u_int16_t *)Value = val & 0xffff;
	break;
    case 32:
	*(u_int32_t *)Value = val;
	break;
    default:
	/* debug trap goes here */
	break;
    }
    

    return(AE_OK);
}


ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID		*PciId,
    UINT32		Register,
    ACPI_INTEGER	Value,
    UINT32		Width)
{
    u_int32_t	byte_width = Width / 8;

    if (!pci_cfgregopen())
    	return(AE_NOT_EXIST);

    pci_cfgregwrite(PciId->Bus, PciId->Device, PciId->Function, Register, Value, byte_width);

    return(AE_OK);
}
