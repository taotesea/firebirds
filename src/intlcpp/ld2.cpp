/*
 *	PROGRAM:	InterBase International support
 *	MODULE:		ld2.c
 *	DESCRIPTION:	Additional Language Driver lookup & support routines
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */


/************************************************************************
 * This file is a template for users to implement a gdsintl2.so library 
 * with their own international character sets in it.
 * It is not part of the 
 *
 */

#include "firebird.h"
#include "../intlcpp/ldcommon.h"
#include "../intlcpp/ld_proto.h"


#ifdef DEV_BUILD
void LD2_assert(filename, lineno)
	 SCHAR *filename;
	 int lineno;
{
/**************************************
 *
 *	L D 2 _ a s s e r t
 *
 **************************************
 *
 * Functional description
 *
 *	Utility function for assert() macro 
 *	Defined locally (clone from jrd/err.c) as ERR_assert isn't
 *	a shared module entry point on all platforms, whereas gds__log is.
 *
 **************************************/
	TEXT buffer[MAXPATHLEN];

	sprintf(buffer, "Assertion failed: INTL file \"%s\", line %d\n", filename,
			lineno);
#if !(defined VMS || defined WIN_NT)
	gds__log(buffer);
#else
	ib_printf(buffer);
#endif
}
#endif


/* Note: Cannot use a lookup table here as SUN4 cannot init pointers inside
 * shared libraries
 */

#define DRIVER(num, name) \
	case (num): \
		*fun = (FPTR_SHORT) (name); \
		return (0);

#define CHARSET_INIT(num, name) \
	case (num): \
		*fun = (FPTR_SHORT) (name); \
		return (0);

#define CONVERT_INIT_BI(to_cs, from_cs, name)\
	if (((parm1 == (to_cs)) && (parm2 == (from_cs))) ||\
	    ((parm2 == (to_cs)) && (parm1 == (from_cs)))) \
	    {\
	    *fun = (FPTR_SHORT) (name);\
	    return (0);\
	    }



USHORT DLL_EXPORT LD2_lookup(USHORT objtype,
                            FPTR_SHORT * fun, SSHORT parm1, SSHORT parm2)
{

   switch (objtype) {
   case type_texttype:
       *fun = NULL;
        return 1;
   case type_charset:
       *fun = NULL;
       return 1;
   case type_csconvert:
       *fun = NULL;
       return 1;
   default:
#ifdef DEV_BUILD
       assert(0);
#endif
       *fun = NULL;
       return 1;
   }
}


#undef DRIVER
#undef CHARSET_INIT
#undef CONVERT_INIT_BI
