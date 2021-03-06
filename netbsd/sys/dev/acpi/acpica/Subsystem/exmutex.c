
/******************************************************************************
 *
 * Module Name: exmutex - ASL Mutex Acquire/Release functions
 *              xRevision: 7 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999, 2000, 2001, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exmutex.c,v 1.3 2001/11/13 13:02:00 lukem Exp $");

#define __EXMUTEX_C__

#include "acpi.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "achware.h"
#include "acevents.h"

#define _COMPONENT          ACPI_EXECUTER
        MODULE_NAME         ("exmutex")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExUnlinkMutex
 *
 * PARAMETERS:  *ObjDesc            - The mutex to be unlinked
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a mutex from the "AcquiredMutex" list
 *
 ******************************************************************************/

void
AcpiExUnlinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{

    if (ObjDesc->Mutex.Next)
    {
        (ObjDesc->Mutex.Next)->Mutex.Prev = ObjDesc->Mutex.Prev;
    }
    if (ObjDesc->Mutex.Prev)
    {
        (ObjDesc->Mutex.Prev)->Mutex.Next = ObjDesc->Mutex.Next;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExLinkMutex
 *
 * PARAMETERS:  *ObjDesc            - The mutex to be linked
 *              *ListHead           - head of the "AcquiredMutex" list
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Add a mutex to the "AcquiredMutex" list for this walk
 *
 ******************************************************************************/

static void
AcpiExLinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *ListHead)
{

    /* This object will be the first object in the list */

    ObjDesc->Mutex.Prev = ListHead;
    ObjDesc->Mutex.Next = ListHead->Mutex.Next;

    /* Update old first object to point back to this object */

    if (ListHead->Mutex.Next)
    {
        (ListHead->Mutex.Next)->Mutex.Prev = ObjDesc;
    }

    /* Update list head */

    ListHead->Mutex.Next = ObjDesc;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExAcquireMutex
 *
 * PARAMETERS:  *TimeDesc           - The 'time to delay' object descriptor
 *              *ObjDesc            - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire an AML mutex
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExAcquireMutex (
    ACPI_OPERAND_OBJECT     *TimeDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;


    FUNCTION_TRACE_PTR ("ExAcquireMutex", ObjDesc);

    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * Current Sync must be less than or equal to the sync level of the
     * mutex.  This mechanism provides some deadlock prevention
     */
    if (WalkState->CurrentSyncLevel > ObjDesc->Mutex.SyncLevel)
    {
        return_ACPI_STATUS (AE_AML_MUTEX_ORDER);
    }

    /*
     * If the mutex is already owned by this thread,
     * just increment the acquisition depth
     */
    if (ObjDesc->Mutex.Owner == WalkState)
    {
        ObjDesc->Mutex.AcquisitionDepth++;
        return_ACPI_STATUS (AE_OK);
    }

    /* Acquire the mutex, wait if necessary */

    Status = AcpiExSystemAcquireMutex (TimeDesc, ObjDesc);
    if (ACPI_FAILURE (Status))
    {
        /* Includes failure from a timeout on TimeDesc */

        return_ACPI_STATUS (Status);
    }

    /* Have the mutex, update mutex and walk info */

    ObjDesc->Mutex.Owner = WalkState;
    ObjDesc->Mutex.AcquisitionDepth = 1;
    WalkState->CurrentSyncLevel = ObjDesc->Mutex.SyncLevel;

    /* Link the mutex to the walk state for force-unlock at method exit */

    AcpiExLinkMutex (ObjDesc, (ACPI_OPERAND_OBJECT *)
                &(WalkState->WalkList->AcquiredMutexList));

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReleaseMutex
 *
 * PARAMETERS:  *ObjDesc            - The object descriptor for this op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release a previously acquired Mutex.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReleaseMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status;


    FUNCTION_TRACE ("ExReleaseMutex");


    if (!ObjDesc)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* The mutex must have been previously acquired in order to release it */

    if (!ObjDesc->Mutex.Owner)
    {
        return_ACPI_STATUS (AE_AML_MUTEX_NOT_ACQUIRED);
    }

    /* The Mutex is owned, but this thread must be the owner */

    if (ObjDesc->Mutex.Owner != WalkState)
    {
        return_ACPI_STATUS (AE_AML_NOT_OWNER);
    }

    /*
     * The sync level of the mutex must be less than or
     * equal to the current sync level
     */
    if (ObjDesc->Mutex.SyncLevel > WalkState->CurrentSyncLevel)
    {
        return_ACPI_STATUS (AE_AML_MUTEX_ORDER);
    }

    /*
     * Match multiple Acquires with multiple Releases
     */
    ObjDesc->Mutex.AcquisitionDepth--;
    if (ObjDesc->Mutex.AcquisitionDepth != 0)
    {
        /* Just decrement the depth and return */

        return_ACPI_STATUS (AE_OK);
    }


    /* Release the mutex */

    Status = AcpiExSystemReleaseMutex (ObjDesc);

    /* Update the mutex and walk state */

    ObjDesc->Mutex.Owner = NULL;
    WalkState->CurrentSyncLevel = ObjDesc->Mutex.SyncLevel;

    /* Unlink the mutex from the owner's list */

    AcpiExUnlinkMutex (ObjDesc);

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExReleaseAllMutexes
 *
 * PARAMETERS:  *MutexList            - Head of the mutex list
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release all mutexes in the list
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExReleaseAllMutexes (
    ACPI_OPERAND_OBJECT     *ListHead)
{
    ACPI_OPERAND_OBJECT     *Next = ListHead->Mutex.Next;
    ACPI_OPERAND_OBJECT     *This;


    FUNCTION_ENTRY ();


    /*
     * Traverse the list of owned mutexes, releasing each one.
     */
    while (Next)
    {
        This = Next;
        Next = This->Mutex.Next;

        /* Mark mutex un-owned */

        This->Mutex.Owner = NULL;
        This->Mutex.Prev = NULL;
        This->Mutex.Next = NULL;
        This->Mutex.AcquisitionDepth = 0;

         /* Release the mutex */

        AcpiExSystemReleaseMutex (This);
    }

    return (AE_OK);
}


