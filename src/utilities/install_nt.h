/*
 *	PROGRAM:	Windows NT installation utilities
 *	MODULE:		install.h
 *	DESCRIPTION:	Defines for Windows NT installation utilities
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

#ifndef _UTILITIES_INSTALL_NT_H_
#define _UTILITIES_INSTALL_NT_H_

#define REMOTE_SERVICE			"FirebirdServerDefaultInstance"
#define REMOTE_DISPLAY_NAME		"Firebird Server - DefaultInstance"
#define REMOTE_DISPLAY_DESCR	"Firebird Database Server - www.firebirdsql.org"
#define REMOTE_SS_EXECUTABLE	"bin\\fbserver"
#define REMOTE_CS_EXECUTABLE	"bin\\fb_inet_server"

#define ISCGUARD_SERVICE		"FirebirdGuardianDefaultInstance"
#define ISCGUARD_DISPLAY_NAME	"Firebird Guardian - DefaultInstance"
#define ISCGUARD_DISPLAY_DESCR	"Firebird Server Guardian - www.firebirdsql.org"
#define ISCGUARD_EXECUTABLE		"bin\\fbguard"
#define GUARDIAN_MUTEX			"FirebirdGuardianMutex"
/* Starting with 128 the service prams are user defined */
#define SERVICE_CREATE_GUARDIAN_MUTEX 128
#define REMOTE_DEPENDENCIES		"Tcpip\0\0"

#define COMMAND_NONE		0
#define COMMAND_INSTALL		1
#define COMMAND_REMOVE		2
#define COMMAND_START		3
#define COMMAND_STOP		4
#define COMMAND_CONFIG		5
#define COMMAND_QUERY		6

#define STARTUP_DEMAND		0
#define STARTUP_AUTO		1

#define NO_GUARDIAN			0
#define USE_GUARDIAN		1

#define DEFAULT_PRIORITY	0
#define NORMAL_PRIORITY		1
#define HIGH_PRIORITY		2

#define ARCH_SS				0
#define ARCH_CS				1

// instclient constants
#define CLIENT_NONE			0
#define CLIENT_FB			1
#define CLIENT_GDS			2

#define GDS32_NAME			"GDS32.DLL"
#define FBCLIENT_NAME		"FBCLIENT.DLL"

// instsvc status codes
#define IB_SERVICE_ALREADY_DEFINED			100
#define IB_SERVICE_RUNNING					101
#define FB_LOGON_SRVC_RIGHT_ALREADY_DEFINED	102
#define FB_SERVICE_STATUS_RUNNING			100
#define FB_SERVICE_STATUS_STOPPED			111
#define FB_SERVICE_STATUS_PENDING			112
#define FB_SERVICE_STATUS_NOT_INSTALLED		113
#define FB_SERVICE_STATUS_UNKNOWN			114

// instclient status codes
#define FB_INSTALL_COPY_REQUIRES_REBOOT		200
#define FB_INSTALL_SAME_VERSION_FOUND		201
#define FB_INSTALL_NEWER_VERSION_FOUND		202
#define FB_INSTALL_FILE_NOT_FOUND 			203
#define FB_INSTALL_CANT_REMOVE_ALIEN_VERSION	204
#define FB_INSTALL_FILE_PROBABLY_IN_USE		205
#define FB_INSTALL_SHARED_COUNT_ZERO  		206

#endif /* _UTILITIES_INSTALL_NT_H_ */
