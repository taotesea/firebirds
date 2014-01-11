/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blb.h
 *	DESCRIPTION:	Blob handling definitions
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
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 */

#ifndef JRD_BLB_H
#define JRD_BLB_H

#include "../jrd/RecordNumber.h"

namespace Jrd {

/* Blob id.  A blob has two states -- temporary and permanent.  In each
   case, the blob id is 8 bytes (2 longwords) long.  In the case of a
   temporary blob, the first word is NULL and the second word points to
   an internal blob block.  In the case of a permanent blob, the first
   word contains the relation id of the blob and the second the record
   number of the first segment-clump.  The two types of blobs can be
   reliably distinguished by a zero or non-zero relation id. */

class BlobControl;
class jrd_req;
class jrd_tra;

// This structure must occupy 8 bytes
struct bid {
	union {
		// Internal decomposition of the structure
		RecordNumber::Packed bid_internal;

		// This is how bid structure represented in public API.
		// Must be present to enforce alignment rules when structure is declared on stack
		struct {
			ULONG bid_quad_high;
			ULONG bid_quad_low;
		} bid_quad;
	};
	
	ULONG& bid_temp_id() {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return bid_internal.bid_temp_id();
	}

	ULONG bid_temp_id() const {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return bid_internal.bid_temp_id();
	}

	bool isEmpty() const { 
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return bid_quad.bid_quad_high == 0 && bid_quad.bid_quad_low == 0; 
	}

	void clear() {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		bid_quad.bid_quad_high = 0;
		bid_quad.bid_quad_low = 0;
	}

	void set_temporary(ULONG temp_id) {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		clear();
		bid_temp_id() = temp_id;
	}

	void set_permanent(USHORT relation_id, RecordNumber num) {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		clear();
		bid_internal.bid_relation_id = relation_id;
		num.bid_encode(&bid_internal);
	}

	RecordNumber get_permanent_number() const {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		RecordNumber temp;
		temp.bid_decode(&bid_internal);
		return temp;
	}

	bool operator == (const bid& other) const {
		// Make sure that compiler packed structure like we wanted
		fb_assert(sizeof(*this) == 8);

		return bid_quad.bid_quad_high == other.bid_quad.bid_quad_high && 
			bid_quad.bid_quad_low == other.bid_quad.bid_quad_low;
	}
};

/* Your basic blob block. */

class blb : public pool_alloc_rpt<UCHAR, type_blb>
{
    public:
	Attachment*	blb_attachment;	/* database attachment */
	jrd_rel*	blb_relation;	/* Relation, if known */
	jrd_tra*	blb_transaction;	/* Parent transaction block */
	blb*		blb_next;		/* Next blob in transaction */
	UCHAR*		blb_segment;	/* Next segment to be addressed */
	BlobControl*	blb_filter;	/* Blob filter control block, if any */
	bid			blb_blob_id;	/* Id of materialized blob */
	vcl*		blb_pages;		/* Vector of pages */
	USHORT blb_pointers;		/* Max pointer on a page */
	USHORT blb_level;			/* Storage type */
	USHORT blb_max_segment;		/* Longest segment */
	USHORT blb_flags;			/* Interesting stuff (see below) */
	USHORT blb_clump_size;		/* Size of data clump */
	USHORT blb_space_remaining;	/* Data space left */
	USHORT blb_max_pages;		/* Max pages in vector */
	USHORT blb_fragment_size;	/* Residual fragment size */
	USHORT blb_source_interp;	/* source interp (for writing) */
	USHORT blb_target_interp;	/* destination interp (for reading) */
	SSHORT blb_sub_type;		/* Blob's declared sub-type */
	ULONG blb_sequence;			/* Blob page sequence */
	ULONG blb_max_sequence;		/* Number of data pages */
	ULONG blb_count;			/* Number of segments */
	ULONG blb_length;			/* Total length of data sans segments */
	ULONG blb_lead_page;		/* First page number */
	ULONG blb_seek;				/* Seek location */
	ULONG blb_temp_id;          // ID of newly created blob in transaction
	/* blb_data must be longword aligned */
	UCHAR blb_data[1];			/* A page's worth of blob */
};

const int BLB_temporary	= 1;			/* Newly created blob */
const int BLB_eof		= 2;			/* This blob is exhausted */
const int BLB_stream	= 4;			/* Stream style blob */
const int BLB_closed	= 8;			/* Temporary blob has been closed */
const int BLB_damaged	= 16;			/* Blob is busted */
const int BLB_seek		= 32;			/* Seek is pending */
const int BLB_user_def	= 64;			/* Blob is user created */
const int BLB_large_scan	= 128;		/* Blob is larger than page buffer cache */

/* Blob levels are:

	0	small blob -- blob "record" is actual data
	1	medium blob -- blob "record" is pointer to pages
	2	large blob -- blob "record" is pointer to pages of pointers
*/

// mapping blob ids for REPLAY
// Useful only with REPLAY_OSRI_API_CALLS_SUBSYSTEM defined.
class blb_map : public pool_alloc<type_map>
{
    public:
	blb_map*	map_next;
	blb*		map_old_blob;
	blb*		map_new_blob;
};

} //namespace Jrd

#endif // JRD_BLB_H
