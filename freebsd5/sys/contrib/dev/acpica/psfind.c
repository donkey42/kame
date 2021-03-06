
/******************************************************************************
 *
 * Module Name: psfind - Parse tree search routine
 *              $Revision: 42 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2002, Intel Corp.
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


#define __PSFIND_C__

#include "acpi.h"
#include "acparser.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psfind")


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetParent
 *
 * PARAMETERS:  Op              - Get the parent of this Op
 *
 * RETURN:      The Parent op.
 *
 * DESCRIPTION: Get op's parent
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT*
AcpiPsGetParent (
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Parent = Op;


    /* Traverse the tree upward (to root if necessary) */

    while (Parent)
    {
        switch (Parent->Common.AmlOpcode)
        {
        case AML_SCOPE_OP:
        case AML_PACKAGE_OP:
        case AML_METHOD_OP:
        case AML_DEVICE_OP:
        case AML_POWER_RES_OP:
        case AML_THERMAL_ZONE_OP:

            return (Parent->Common.Parent);

        default:
            break;
        }

        Parent = Parent->Common.Parent;
    }

    return (Parent);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsFindName
 *
 * PARAMETERS:  Scope           - Scope to search
 *              Name            - ACPI name to search for
 *              Opcode          - Opcode to search for
 *
 * RETURN:      Op containing the name
 *
 * DESCRIPTION: Find name segment from a list of acpi_ops.  Searches a single
 *              scope, no more.
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
AcpiPsFindName (
    ACPI_PARSE_OBJECT       *Scope,
    UINT32                  Name,
    UINT32                  Opcode)
{
    ACPI_PARSE_OBJECT       *Op;
    ACPI_PARSE_OBJECT       *Field;
    const ACPI_OPCODE_INFO  *OpInfo;


    /* search scope level for matching name segment */

    Op = AcpiPsGetChild (Scope);

    while (Op)
    {
        OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);

        /* Check AML_CREATE first (since some opcodes have AML_FIELD set also )*/

        if (OpInfo->Flags & AML_CREATE)
        {
            if (Op->Common.AmlOpcode == AML_CREATE_FIELD_OP)
            {
                Field = AcpiPsGetArg (Op, 3);
            }
            else
            {
                /* CreateXXXField, check name */

                Field = AcpiPsGetArg (Op, 2);
            }

            if ((Field) &&
                (Field->Common.Value.String) &&
                (!ACPI_STRNCMP (Field->Common.Value.String, (char *) &Name, ACPI_NAME_SIZE)))
            {
                return (Op);
            }
        }
        else if (OpInfo->Flags & AML_FIELD)
        {
            /* Field, search named fields */

            Field = AcpiPsGetChild (Op);
            while (Field)
            {
                OpInfo = AcpiPsGetOpcodeInfo (Field->Common.AmlOpcode);

                if ((OpInfo->Flags & AML_NAMED) &&
                   AcpiPsGetName (Field) == Name &&
                   (!Opcode || Field->Common.AmlOpcode == Opcode))
                {
                    return (Field);
                }

                Field = Field->Common.Next;
            }
        }
        else if ((OpInfo->Flags & AML_NAMED) &&
                 (AcpiPsGetName (Op) == Name) &&
                 (!Opcode || Op->Common.AmlOpcode == Opcode || Opcode == AML_SCOPE_OP))
        {
            break;
        }

        Op = Op->Common.Next;
    }

    return (Op);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsFind
 *
 * PARAMETERS:  Scope           - Where to begin the search
 *              Path            - ACPI Path to the named object
 *              Opcode          - Opcode associated with the object
 *              Create          - if TRUE, create the object if not found.
 *
 * RETURN:      Op if found, NULL otherwise.
 *
 * DESCRIPTION: Find object within scope
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT*
AcpiPsFind (
    ACPI_PARSE_OBJECT       *Scope,
    NATIVE_CHAR             *Path,
    UINT16                  Opcode,
    UINT32                  Create)
{
    UINT32                  SegCount;
    UINT32                  Name;
    UINT32                  NameOp;
    ACPI_PARSE_OBJECT       *Op = NULL;
    BOOLEAN                 Unprefixed = TRUE;


    ACPI_FUNCTION_TRACE_PTR ("PsFind", Scope);


    if (!Scope || !Path)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "Null path (%p) or scope (%p)!\n",
            Path, Scope));
        return_PTR (NULL);
    }

    AcpiGbl_PsFindCount++;

#if 0 
    if ((Create) && (Opcode == AML_SCOPE_OP))
    {
        Op = AcpiPsAllocOp (AML_SCOPE_OP);
        if (Op)
        {
            AcpiPsSetName (Op, 'XXXX');
            AcpiPsAppendArg (Scope, Op);
        }
/*        return_PTR (Op);*/
    }
#endif

    /* Handle all prefixes in the name path */

    while (AcpiPsIsPrefixChar (ACPI_GET8 (Path)))
    {
        switch (ACPI_GET8 (Path))
        {
        case '\\':

            /* Could just use a global for "root scope" here */

            while (Scope->Common.Parent)
            {
                Scope = Scope->Common.Parent;
            }
            break;


        case '^':

            /* Go up to the next valid scoping Op (method, scope, etc.) */

            if (AcpiPsGetParent (Scope))
            {
                Scope = AcpiPsGetParent (Scope);
            }
            break;

        default:
            /* Should not get here */
            break;
        }

        Unprefixed = FALSE;
        Path++;
    }


    /* get name segment count */

    switch (ACPI_GET8 (Path))
    {
    case 0:
        SegCount = 0;

        /* Null name case */

        if (Unprefixed)
        {
            Op = NULL;
        }
        else
        {
            Op = Scope;
        }


        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
            "Null path, returning current root scope Op=%p\n", Op));
        return_PTR (Op);

    case AML_DUAL_NAME_PREFIX:
        SegCount = 2;
        Path++;
        break;

    case AML_MULTI_NAME_PREFIX_OP:
        SegCount = ACPI_GET8 (Path + 1);
        Path += 2;
        break;

    default:
        SegCount = 1;
        break;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
        "Search scope %p Segs=%d Opcode=%4.4hX Create=%d\n",
        Scope, SegCount, Opcode, Create));


    /* match each name segment */

    while (Scope && SegCount)
    {
        ACPI_MOVE_UNALIGNED32_TO_32 (&Name, Path);
        Path += 4;
        SegCount --;

        if (SegCount)
        {
            NameOp = 0;
        }
        else
        {
            NameOp = Opcode;
        }

        Op = AcpiPsFindName (Scope, Name, NameOp);
        if (Op)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                "[%4.4s] Found! Op=%p Opcode=%4.4hX\n",
                (char *) &Name, Op, Op->Common.AmlOpcode));
        }

        if (!Op)
        {
            if (Create)
            {
                /* Create a new Scope level */

                if (SegCount)
                {
                    Op = AcpiPsAllocOp (AML_SCOPE_OP);
                }
                else
                {
                    Op = AcpiPsAllocOp (Opcode);
                }

                if (Op)
                {
                    AcpiPsSetName (Op, Name);
                    AcpiPsAppendArg (Scope, Op);

                    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                        "[%4.4s] Not found, created Op=%p Opcode=%4.4hX\n",
                        (char *) &Name, Op, Opcode));
                }
            }

            else if (Unprefixed)
            {
                /* Search higher scopes for unprefixed name */

                while (!Op && Scope->Common.Parent)
                {
                    Scope = Scope->Common.Parent;
                    Op = AcpiPsFindName (Scope, Name, Opcode);
                    if (Op)
                    {
                        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                            "[%4.4s] Found in parent tree! Op=%p Opcode=%4.4hX\n",
                            (char *) &Name, Op, Op->Common.AmlOpcode));
                    }
                    else
                    {
                        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                            "[%4.4s] Not found in parent=%p\n",
                            (char *) &Name, Scope));
                    }
                }
            }
            else
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_PARSE,
                    "Segment [%4.4s] Not Found in scope %p!\n",
                    (char *) &Name, Scope));
            }
        }

        Unprefixed = FALSE;
        Scope = Op;
    }

    return_PTR (Op);
}


