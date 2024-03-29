/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		inet_proto.h
 *	DESCRIPTION:	Prototpe header file for inet.c
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

#ifndef _REMOTE_INET_PROTO_H_
#define _REMOTE_INET_PROTO_H_

#ifdef __cplusplus
extern "C" {
#endif

extern PORT	INET_analyze (TEXT *, USHORT *, ISC_STATUS *, TEXT *, TEXT *, USHORT, SCHAR*, SSHORT);
extern PORT	DLL_EXPORT INET_connect (TEXT *, struct packet *, ISC_STATUS *, USHORT, SCHAR*, SSHORT);
extern PORT	INET_reconnect (HANDLE, TEXT *, ISC_STATUS *);
extern PORT	DLL_EXPORT INET_server (int);
extern void	INET_set_clients (int);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif /* _REMOTE_INET_PROTO_H */ 
