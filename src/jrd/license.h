/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		license.h
 *	DESCRIPTION:	Internal licensing parameters
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
 * $Id: license.h,v 1.30.2.3 2007-12-17 14:48:38 paulbeach Exp $
 * Revision 1.5  2000/12/08 16:18:21  fsg
 * Preliminary changes to get IB_BUILD_NO automatically
 * increased on commits.
 *
 * setup_dirs will create 'jrd/build_no.h' by a call to
 * a slightly modified 'builds_win32/original/build_no.ksh'
 * that gets IB_BUILD_NO from 'this_build', that hopefully
 * will be increased automatically in the near future :-)
 *
 * I have changed 'jrd/iblicense.h' to use IB_BUILD_TYPE
 * from 'jrd/build_no.h'.
 * So all changes to version numbers, build types etc. can
 * now be done in 'builds_win32/original/build_no.ksh'.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "MAC" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "XENIX" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "NCR3000" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "M88K" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "HP9000 s300" port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix/MIPS" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "ALPHA_NT" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "HP700" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2002.10.30 Sean Leyne - Code Cleanup, removed obsolete "SUN3_3" port
 *
 */

#ifndef _JRD_LICENSE_H_
#define _JRD_LICENSE_H_

#include "../jrd/build_no.h"
#include "../jrd/isc_version.h"

#ifdef HPUX
#ifdef HP10
#define FB_PLATFORM	"HU"
#endif /* HP10 */
#ifdef HP11
#define FB_PLATFORM "HU"
#endif /* HP11 */
#endif

#ifdef sun
#ifdef sparc
#ifdef SOLARIS
#define FB_PLATFORM	"SO"
#else
#define FB_PLATFORM	"S4"
#endif /* Solaris */
#endif /* Sparc */
#ifdef i386
#define FB_PLATFORM     "SI"
#endif /* i386 */
#ifndef FB_PLATFORM
#define FB_PLATFORM	"S3"
#endif
#endif /* sun */

#ifdef VMS
#ifdef __ALPHA
#define FB_PLATFORM     "AV"
#else
#define FB_PLATFORM	"VM"
#endif
#endif

#ifdef AIX_RS6000
#define FB_PLATFORM	"IA"
#endif

#ifdef AIX_PPC
#define FB_PLATFORM	"PA"
#endif

#ifdef WIN_NT
#ifdef i386
#define FB_PLATFORM	"WI"
#else
#define FB_PLATFORM	"NP"
#endif /* i386 */
#endif

#ifdef LINUX
#define FB_PLATFORM     "LI"	/* Linux on Intel */
#endif

#ifdef SINIXZ
#define FB_PLATFORM     "SZ"	/* SINIX for PC  */
#endif

#ifdef FREEBSD
#define FB_PLATFORM     "FB"	/* FreeBSD/i386 */
#endif

#ifdef NETBSD
#define FB_PLATFORM     "NB"	/* NetBSD */
#endif

#ifdef DARWIN
#ifdef i386
#define FB_PLATFORM	"UI"	/* Darwin/Intel */
#endif
#endif
#if defined(DARWIN) && (__ppc__)
#define FB_PLATFORM     "UP"	/* Darwin/PowerPC */
#endif

#ifndef FB_VERSION
#define FB_VERSION      FB_PLATFORM "-" FB_BUILD_TYPE FB_MAJOR_VER "." FB_MINOR_VER "." FB_REV_NO "." FB_BUILD_NO " " FB_BUILD_SUFFIX
#endif

#ifndef GDS_VERSION
#define GDS_VERSION		FB_VERSION
#endif

#ifndef ISC_VERSION
#define ISC_VERSION		FB_PLATFORM "-" FB_BUILD_TYPE ISC_MAJOR_VER "." ISC_MINOR_VER "." FB_REV_NO "." FB_BUILD_NO " " FB_BUILD_SUFFIX
#endif

#endif /* _JRD_LICENSE_H_ */

