/*
 *	PROGRAM:	Windows NT installation utilities
 *	MODULE:		registry.h
 *	DESCRIPTION:	Defines for the registry
 *
 * The contents of this file are subject to the Independant Developers 
 * Public License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.ibphoenix.com/IDPL.html
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
 * Contributor(s): Paul Reeves__________________________________.
 */


#ifndef _REGISTRY_DEFINES_
#define _REGISTRY_DEFINES_

#define REG_KEY_BOR_ROOT			"SOFTWARE\\Borland\\InterBase"
#define REG_KEY_BOR_ROOT_CUR_VER	"SOFTWARE\\Borland\\InterBase\\CurrentVersion"

#define REG_KEY_FIR_ROOT			"SOFTWARE\\FirebirdSQL\\Firebird"
#define REG_KEY_FIR_ROOT_CUR_VER	"SOFTWARE\\FirebirdSQL\\Firebird\\CurrentVersion"

#define REG_KEY_ROOT				REG_KEY_FIR_ROOT
#define REG_KEY_ROOT_CUR_VER		REG_KEY_FIR_ROOT_CUR_VER

#endif /* _REGISTRY_DEFINES_ */
