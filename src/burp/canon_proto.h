/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		canon_proto.h		
 *	DESCRIPTION:	Prototype Header file for canonical.c
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

#ifndef _BURP_CANON_PROTO_H_
#define _BURP_CANON_PROTO_H_

#ifdef __cplusplus
extern "C" {
#endif

extern ULONG	CAN_encode_decode (struct burp_rel *, struct lstring *, UCHAR *, int);
extern ULONG	CAN_slice (struct lstring *, struct lstring *, int, USHORT, UCHAR *);

#ifdef __cplusplus
}	/* extern "C" */
#endif

#endif	/* _BURP_CANON_PROTO_H_ */
