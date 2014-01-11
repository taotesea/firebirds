/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		unix.cpp
 *	DESCRIPTION:	UNIX (BSD) specific physical IO
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.27 Sean Leyne - Completed removal of "DELTA" port
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_AIO_H
#include <aio.h>
#endif

#include "../jrd/jrd.h"
#include "../jrd/os/pio.h"
#include "../jrd/ods.h"
#include "../jrd/lck.h"
#include "../jrd/cch.h"
#include "../jrd/ibase.h"
#include "gen/iberror.h"
#include "../jrd/all_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/ods_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/thd.h"

using namespace Jrd;

// Some operating systems have problems with use of write/read with 
// big (>2Gb) files. On the other hand, pwrite/pread works fine for them.
// Therefore:
#if defined SOLARIS
#define BROKEN_IO_64
#endif
// which will force use of pread/pwrite even for CS.

/* SUPERSERVER uses a mutex to allow atomic seek/read(write) sequences.
   When possible, it uses "positioned" read (write) calls to avoid a seek
   and allow multiple threads to overlap database I/O. This functions also
   help at some OSs with broken read/write calls. */
#if defined SUPERSERVER || defined BROKEN_IO_64
#if (defined PREAD && defined PWRITE) || defined HAVE_AIO_H
#define PREAD_PWRITE
#endif
#endif

#ifdef SUPERSERVER
#define THD_IO_MUTEX_INIT(mutx)
#define THD_IO_MUTEX_LOCK(mutx)		mutx->enter()
#define THD_IO_MUTEX_UNLOCK(mutx)	mutx->leave()
#define THD_IO_MUTEX_DESTROY(mutx)
#else
#define THD_IO_MUTEX_INIT(mutx)
#define THD_IO_MUTEX_LOCK(mutx)
#define THD_IO_MUTEX_UNLOCK(mutx)
#define THD_IO_MUTEX_DESTROY(mutx)
#endif

#define IO_RETRY	20

#ifdef O_SYNC
#define SYNC		O_SYNC
#endif

    /* Changed to not redfine SYNC if O_SYNC already exists
       they seem to be the same values anyway. MOD 13-07-2001 */
#if (!(defined SYNC) && (defined O_FSYNC))
#define SYNC		O_FSYNC
#endif

#ifdef O_DSYNC
#undef SYNC
#define SYNC		O_DSYNC
#endif

#ifndef SYNC
#define SYNC		0
#endif

#ifndef O_BINARY
#define O_BINARY	0
#endif

#ifdef SUPERSERVER
#define MASK		0600
#else
#define MASK		0660
#endif

#define FCNTL_BROKEN
// please undefine FCNTL_BROKEN for operating systems,
// that can successfully change BOTH O_DIRECT and O_SYNC using fcntl()

static jrd_file* seek_file(jrd_file*, BufferDesc*, FB_UINT64 *, ISC_STATUS *);
static jrd_file* setup_file(Database*, const Firebird::PathName&, int);
static bool unix_error(TEXT*, jrd_file*, ISC_STATUS, ISC_STATUS*);
#if defined PREAD_PWRITE && !(defined HAVE_PREAD && defined HAVE_PWRITE)
static SLONG pread(int, SCHAR *, SLONG, SLONG);
static SLONG pwrite(int, SCHAR *, SLONG, SLONG);
#endif
#ifdef SUPPORT_RAW_DEVICES
static bool raw_devices_validate_database (int, const Firebird::PathName&);
static int  raw_devices_unlink_database (const Firebird::PathName&);
#endif
static int openFile(const char*, bool, bool);
static bool maybe_close_file(int&);


int PIO_add_file(Database* dbb, jrd_file* main_file, const Firebird::PathName& file_name, SLONG start)
{
/**************************************
 *
 *	P I O _ a d d _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Add a file to an existing database.  Return the sequence
 *	number of the new file.  If anything goes wrong, return a
 *	sequence of 0.
 *	NOTE:  This routine does not lock any mutexes on
 *	its own behalf.  It is assumed that mutexes will
 *	have been locked before entry.
 *
 **************************************/
	jrd_file* new_file = PIO_create(dbb, file_name, false, false);
	if (!new_file)
		return 0;

	new_file->fil_min_page = start;
	USHORT sequence = 1;

	jrd_file* file;
	for (file = main_file; file->fil_next; file = file->fil_next)
		++sequence;

	file->fil_max_page = start - 1;
	file->fil_next = new_file;

	return sequence;
}


void PIO_close(jrd_file* main_file)
{
/**************************************
 *
 *	P I O _ c l o s e
 *
 **************************************
 *
 * Functional description
 *	NOTE:  This routine does not lock any mutexes on
 *	its own behalf.  It is assumed that mutexes will
 *	have been locked before entry.
 *
 **************************************/

	for (jrd_file* file = main_file; file; file = file->fil_next) {
		if (file->fil_desc && file->fil_desc != -1) {
			close(file->fil_desc);
			file->fil_desc = -1;
#ifndef PREAD_PWRITE
			THD_IO_MUTEX_DESTROY(file->fil_mutex);
#endif
		}
	}
}


int PIO_connection(const Firebird::PathName& file_name)
{
/**************************************
 *
 *	P I O _ c o n n e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Analyze a file specification and determine whether a page/lock
 *	server is required and available.  If so, return a "connection"
 *	block.  If not, return NULL.
 *
 *	Note: The file name must have been expanded prior to this call.
 *
 **************************************/

	return 0;
}


jrd_file* PIO_create(Database* dbb, const Firebird::PathName& file_name, bool overwrite, bool share_delete)
{
/**************************************
 *
 *	P I O _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Create a new database file.
 *	NOTE:  This routine does not lock any mutexes on
 *	its own behalf.  It is assumed that mutexes will
 *	have been locked before entry.
 *
 **************************************/
#ifdef SUPERSERVER_V2
	const int flag =
		SYNC | O_RDWR | O_CREAT | (overwrite ? O_TRUNC : O_EXCL) | O_BINARY;
#else
#ifdef SUPPORT_RAW_DEVICES
	const int flag = O_RDWR |
			(PIO_on_raw_device(file_name) ? 0 : O_CREAT) |
			(overwrite ? O_TRUNC : O_EXCL) |
			O_BINARY;
#else
	const int flag = O_RDWR | O_CREAT | (overwrite ? O_TRUNC : O_EXCL) | O_BINARY;
#endif
#endif

	const int desc = open(file_name.c_str(), flag, 0666);
	if (desc == -1) 
	{
		ERR_post(isc_io_error,
				 isc_arg_string, "open O_CREAT",
				 isc_arg_cstring, file_name.length(), ERR_string(file_name),
				 isc_arg_gds, isc_io_create_err, isc_arg_unix, errno, isc_arg_end);
	}
#ifdef HAVE_FCHMOD
	if (fchmod(desc, MASK) < 0)
#else
	if (chmod(file_name.c_str(), MASK) < 0)
#endif
	{
		ERR_post(isc_io_error,
				 isc_arg_string, "chmod",
				 isc_arg_cstring, file_name.length(), ERR_string(file_name),
				 isc_arg_gds, isc_io_create_err, isc_arg_unix, errno, isc_arg_end);
	}

/* File open succeeded.  Now expand the file name. */

	Firebird::PathName expanded_name(file_name);
	ISC_expand_filename(expanded_name, false);
	jrd_file* file;
	try 
	{
		file = setup_file(dbb, expanded_name, desc);
	} 
	catch(const std::exception&) 
	{
		close(desc);
		throw;
	}
	return file;
}


int PIO_expand(const TEXT* file_name, USHORT file_length, TEXT* expanded_name, size_t len_expanded)
{
/**************************************
 *
 *	P I O _ e x p a n d
 *
 **************************************
 *
 * Functional description
 *	Fully expand a file name.  If the file doesn't exist, do something
 *	intelligent.
 *
 **************************************/

	return ISC_expand_filename(file_name, file_length, 
		expanded_name, len_expanded, false);
}


void PIO_flush(jrd_file* main_file)
{
/**************************************
 *
 *	P I O _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Flush the operating system cache back to good, solid oxide.
 *
 **************************************/

/* Since all SUPERSERVER_V2 database and shadow I/O is synchronous, this
   is a no-op. */

#ifndef SUPERSERVER_V2
	for (jrd_file* file = main_file; file; file = file->fil_next) {
		if (file->fil_desc != -1) {	/* This really should be an error */
			THD_IO_MUTEX_LOCK(file->fil_mutex);
			fsync(file->fil_desc);
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
		}
	}
#endif
}


static int openFile(const char* name, bool forcedWrites, bool readOnly)
{
/**************************************
 *
 *	o p e n F i l e
 *
 **************************************
 *
 * Functional description
 *	Open a file with appropriate flags.
 *
 **************************************/

	int flag = O_BINARY | (readOnly ? O_RDONLY : O_RDWR);
#ifdef SUPERSERVER_V2
	flag |= SYNC;
	// what to do with O_DIRECT here ?
#else
	if (forcedWrites)
		flag |= SYNC;
#endif

	for (int i = 0; i < IO_RETRY; i++)
	{
		int desc = open(name, flag);
		if (desc != -1)
			return desc;
		if (!SYSCALL_INTERRUPTED(errno))
			break;
	}

	return -1;
}


static bool maybe_close_file(int& desc)
{
/**************************************
 *
 * M a y b e C l o s e F i l e
 *
 **************************************
 *
 * Functional description
 * If the file is open, close it.
 *
 **************************************/

	if (desc >= 0)
	{
		close(desc);
		desc = -1;
		return true;
	}
	return false;
}


void PIO_force_write(jrd_file* file, bool forcedWrites)
{
/**************************************
 *
 *	P I O _ f o r c e _ w r i t e	( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Set (or clear) force write, if possible, for the database.
 *
 **************************************/

// Since all SUPERSERVER_V2 database and shadow I/O is synchronous, this is a no-op. 

#ifndef SUPERSERVER_V2
	const bool oldForce = (file->fil_flags & FIL_force_write) != 0;

	if (forcedWrites != oldForce)
	{
		const int control = forcedWrites ? SYNC : 0;

#ifndef FCNTL_BROKEN
		if (fcntl(file->fil_desc, F_SETFL, control) == -1)
		{
			ERR_post(isc_io_error,
					 isc_arg_string, "fcntl() SYNC/DIRECT",
					 isc_arg_cstring, file->fil_length,
					 ERR_string(file->fil_string, file->fil_length), isc_arg_gds,
					 isc_io_access_err, isc_arg_unix, errno, isc_arg_end);
		}
#else //FCNTL_BROKEN
		maybe_close_file(file->fil_desc);
		file->fil_desc = openFile(file->fil_string, forcedWrites, file->fil_flags & FIL_readonly);
		if (file->fil_desc == -1)
		{
			ERR_post(isc_io_error, isc_arg_string, "re open() for SYNC/DIRECT",
					 isc_arg_cstring, file->fil_length, ERR_string(file->fil_string, file->fil_length),
					 isc_arg_gds, isc_io_open_err, isc_arg_unix, errno, isc_arg_end);
		}
#endif //FCNTL_BROKEN

		file->fil_flags &= ~FIL_force_write;
		file->fil_flags |= (forcedWrites ? FIL_force_write : 0);
	}
#endif
}


void PIO_header(Database* dbb, SCHAR * address, int length)
{
/**************************************
 *
 *	P I O _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Read the page header.  This assumes that the file has not been
 *	repositioned since the file was originally mapped.
 *
 **************************************/
	int i;
	FB_UINT64 bytes;

	jrd_file* file = dbb->dbb_file;

	SignalInhibit siHolder;

	if (file->fil_desc == -1)
		unix_error("PIO_header", file, isc_io_read_err, 0);

	for (i = 0; i < IO_RETRY; i++) {
#ifndef PREAD_PWRITE
		THD_IO_MUTEX_LOCK(file->fil_mutex);

		if ((lseek(file->fil_desc, LSEEK_OFFSET_CAST 0, 0)) == -1) {
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
			unix_error("lseek", file, isc_io_read_err, 0);
		}
#endif
#ifdef ISC_DATABASE_ENCRYPTION
		if (dbb->dbb_encrypt_key) {
			SLONG spare_buffer[MAX_PAGE_SIZE / sizeof(SLONG)];

#ifdef PREAD_PWRITE
			if ((bytes = pread(file->fil_desc, spare_buffer, length, 0)) ==
				(FB_UINT64) -1) {
#else
			if ((bytes = read(file->fil_desc, spare_buffer, length)) == 
				(FB_UINT64) -1) {
				THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
				if (SYSCALL_INTERRUPTED(errno))
					continue;
				unix_error("read", file, isc_io_read_err, 0);
			}

			(*dbb->dbb_decrypt) (dbb->dbb_encrypt_key->str_data,
								 spare_buffer, length, address);
		}
		else
#endif /* ISC_DATABASE_ENCRYPTION */
#ifdef PREAD_PWRITE
		if ((bytes = pread(file->fil_desc, address, length, 0)) == (FB_UINT64) -1) {
#else
		if ((bytes = read(file->fil_desc, address, length)) == (FB_UINT64) -1) {
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
			if (SYSCALL_INTERRUPTED(errno))
				continue;
			unix_error("read", file, isc_io_read_err, 0);
		}
		else
			break;
	}

	if (i == IO_RETRY) {
		if (bytes == 0) {
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_header: an empty page read!\n");
			fflush(stderr);
#endif
		}
		else {
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_header: retry count exceeded\n");
			fflush(stderr);
#endif
			unix_error("read_retry", file, isc_io_read_err, 0);
		}
	}
#ifndef PREAD_PWRITE
	THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
}


SLONG PIO_max_alloc(Database* dbb)
{
/**************************************
 *
 *	P I O _ m a x _ a l l o c
 *
 **************************************
 *
 * Functional description
 *	Compute last physically allocated page of database.
 *
 **************************************/
	jrd_file* file;

	for (file = dbb->dbb_file; file->fil_next; file = file->fil_next);

	if (file->fil_desc == -1) {
		unix_error("fstat", file, isc_io_access_err, 0);
		return (0);
	}

	struct stat statistics;
	if (fstat(file->fil_desc, &statistics)) {
		unix_error("fstat", file, isc_io_access_err, 0);
	}

	FB_UINT64 length = statistics.st_size;

/****
#ifndef sun
length = statistics.st_size;
#else
length = statistics.st_blocks * statistics.st_blksize;
#endif
****/

	return file->fil_min_page - file->fil_fudge +
		(length + dbb->dbb_page_size - 1) / dbb->dbb_page_size;
}


SLONG PIO_act_alloc(Database* dbb)
{
/**************************************
 *
 *	P I O _ a c t  _ a l l o c
 *
 **************************************
 *
 * Functional description
 *  Compute actual number of physically allocated pages of database.
 *
 **************************************/
	struct stat statistics;
	ULONG tot_pages = 0;

/**
 **  Traverse the linked list of files and add up the number of pages
 **  in each file
 **/
	for (jrd_file* file = dbb->dbb_file; file != NULL; file = file->fil_next) {
		if (file->fil_desc == -1) {
			unix_error("fstat", file, isc_io_access_err, 0);
			return (0);
		}

		if (fstat(file->fil_desc, &statistics)) {
			unix_error("fstat", file, isc_io_access_err, 0);
		}

		FB_UINT64 length = statistics.st_size;

		tot_pages += (length + dbb->dbb_page_size - 1) / dbb->dbb_page_size;
	}

	return tot_pages;
}




jrd_file* PIO_open(Database* dbb,
			 const Firebird::PathName& string,
			 bool trace_flag,
			 blk* connection, 
			 const Firebird::PathName& file_name,
			 bool share_delete)
{
/**************************************
 *
 *	P I O _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a database file.  If a "connection" block is provided, use
 *	the connection to communication with a page/lock server.
 *
 **************************************/
	bool readOnly = false;
	const TEXT* ptr = (string.hasData() ? string : file_name).c_str();
	int desc = openFile(ptr, false, false);

	if (desc == -1) {
		/* Try opening the database file in ReadOnly mode. The database file could
		 * be on a RO medium (CD-ROM etc.). If this fileopen fails, return error.
		 */
		desc = openFile(ptr, false, true);
		if (desc == -1) {
			ERR_post(isc_io_error,
					 isc_arg_string, "open",
					 isc_arg_cstring, file_name.length(), ERR_cstring(file_name),
					 isc_arg_gds, isc_io_open_err, isc_arg_unix, errno, isc_arg_end);
		}
		else {
			/* If this is the primary file, set Database flag to indicate that it is
			 * being opened ReadOnly. This flag will be used later to compare with
			 * the Header Page flag setting to make sure that the database is set
			 * ReadOnly.
			 */
			if (!dbb->dbb_file)
				dbb->dbb_flags |= DBB_being_opened_read_only;
			readOnly = true;
		}
	}

#ifdef SUPPORT_RAW_DEVICES
	/* At this point the file has successfully been opened in either RW or RO
	 * mode. Check if it is a special file (i.e. raw block device) and if a
	 * valid database is on it. If not, return an error.
	 */
	if (PIO_on_raw_device(file_name)
		&& !raw_devices_validate_database(desc, file_name))
	{
		ERR_post (isc_io_error,
					isc_arg_string, "open",
					isc_arg_cstring, file_name.length(),
						ERR_cstring (file_name),
					isc_arg_gds, isc_io_open_err,
					isc_arg_unix, ENOENT, isc_arg_end);
	}
#endif /* SUPPORT_RAW_DEVICES */

	jrd_file *file;
	try {
		file = setup_file(dbb, string, desc);
		if (readOnly)
			file->fil_flags |= FIL_readonly;
	} catch(const std::exception&) {
		close(desc);
		throw;
	}
	return file;
}


bool PIO_read(jrd_file* file, BufferDesc* bdb, Ods::pag* page, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	P I O _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a data page.  Oh wow.
 *
 **************************************/
	int i;
	FB_UINT64 bytes, offset;

	SignalInhibit siHolder;

	if (file->fil_desc == -1)
		return unix_error("read", file, isc_io_read_err, status_vector);

	Database* dbb = bdb->bdb_dbb;
	const FB_UINT64 size = dbb->dbb_page_size;

#ifdef ISC_DATABASE_ENCRYPTION
	if (dbb->dbb_encrypt_key) {
		SLONG spare_buffer[MAX_PAGE_SIZE / sizeof(SLONG)];

		for (i = 0; i < IO_RETRY; i++) {
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
#ifdef PREAD_PWRITE
            if ((bytes = pread (file->fil_desc, spare_buffer, size, LSEEK_OFFSET_CAST offset)) == size) 
#else
			if ((bytes = read(file->fil_desc, spare_buffer, size)) == size)
#endif
			{
				(*dbb->dbb_decrypt) (dbb->dbb_encrypt_key->str_data,
									 spare_buffer, size, page);
				break;
			}
#ifndef PREAD_PWRITE
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
			if (bytes == -1U && !SYSCALL_INTERRUPTED(errno))
				return unix_error("read", file, isc_io_read_err,
								  status_vector);
		}
	}
	else
#endif /* ISC_DATABASE_ENCRYPTION */
	{
		for (i = 0; i < IO_RETRY; i++) {
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
#ifdef PREAD_PWRITE
			if ((bytes = pread(file->fil_desc, page, size, LSEEK_OFFSET_CAST offset)) == size)
				break;
#else
			if ((bytes = read(file->fil_desc, page, size)) == size)
				break;
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
			if (bytes == -1U && !SYSCALL_INTERRUPTED(errno))
				return unix_error("read", file, isc_io_read_err,
								  status_vector);
		}
	}

#ifndef PREAD_PWRITE
	THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif

	if (i == IO_RETRY) {
		if (bytes == 0) {
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_read: an empty page read!\n");
			fflush(stderr);
#endif
		}
		else {
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_read: retry count exceeded\n");
			fflush(stderr);
#endif
			unix_error("read_retry", file, isc_io_read_err, 0);
		}
	}

	return true;
}


bool PIO_write(jrd_file* file, BufferDesc* bdb, Ods::pag* page, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	P I O _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Write a data page.  Oh wow.
 *
 **************************************/
	int i;
	SLONG bytes;
    FB_UINT64 offset;

	SignalInhibit siHolder;

	if (file->fil_desc == -1)
		return unix_error("write", file, isc_io_write_err, status_vector);

	Database* dbb = bdb->bdb_dbb;
	const SLONG size = dbb->dbb_page_size;

#ifdef ISC_DATABASE_ENCRYPTION
	if (dbb->dbb_encrypt_key) {
		SLONG spare_buffer[MAX_PAGE_SIZE / sizeof(SLONG)];

		(*dbb->dbb_encrypt) (dbb->dbb_encrypt_key->str_data,
							 page, size, spare_buffer);

		for (i = 0; i < IO_RETRY; i++) {
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
#ifdef PREAD_PWRITE
			if ((bytes = pwrite(file->fil_desc, spare_buffer, size, LSEEK_OFFSET_CAST offset)) == size)
				break;
#else
			if ((bytes = write(file->fil_desc, spare_buffer, size)) == size)
				break;
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
			if (bytes == -1U && !SYSCALL_INTERRUPTED(errno))
				return unix_error("write", file, isc_io_write_err,
								  status_vector);
		}
	}
	else
#endif /* ISC_DATABASE_ENCRYPTION */
	{
		for (i = 0; i < IO_RETRY; i++) {
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
#ifdef PREAD_PWRITE
			if ((bytes = pwrite(file->fil_desc, page, size, LSEEK_OFFSET_CAST offset)) == size)
				break;
#else
			if ((bytes = write(file->fil_desc, page, size)) == size)
				break;
			THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif
			if (bytes == (SLONG) -1 && !SYSCALL_INTERRUPTED(errno))
				return unix_error("write", file, isc_io_write_err,
								  status_vector);
		}
	}

#ifndef PREAD_PWRITE
	THD_IO_MUTEX_UNLOCK(file->fil_mutex);
#endif

	return true;
}


static jrd_file* seek_file(jrd_file* file, BufferDesc* bdb, FB_UINT64* offset,
	ISC_STATUS* status_vector)
{
/**************************************
 *
 *	s e e k _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Given a buffer descriptor block, find the appropropriate
 *	file block and seek to the proper page in that file.
 *
 **************************************/
	Database* dbb = bdb->bdb_dbb;
	ULONG page = bdb->bdb_page;

	for (;; file = file->fil_next)
	{
		if (!file) {
			CORRUPT(158);		/* msg 158 database file not available */
		}
		else if (page >= file->fil_min_page && page <= file->fil_max_page)
			break;
	}

	if (file->fil_desc == -1)
	{
		unix_error("lseek", file, isc_io_access_err, status_vector);
		return 0;
	}

	page -= file->fil_min_page - file->fil_fudge;

    FB_UINT64 lseek_offset = page;
    lseek_offset *= dbb->dbb_page_size;

    if (lseek_offset != (FB_UINT64) LSEEK_OFFSET_CAST lseek_offset)
	{
		unix_error("lseek", file, isc_io_32bit_exceeded_err, status_vector);
		return 0;
    }

#ifdef PREAD_PWRITE
	*offset = lseek_offset;
#else
	THD_IO_MUTEX_LOCK(file->fil_mutex);

	if ((lseek(file->fil_desc, LSEEK_OFFSET_CAST lseek_offset, 0)) == (off_t)-1)
	{
		THD_IO_MUTEX_UNLOCK(file->fil_mutex);
		unix_error("lseek", file, isc_io_access_err, status_vector);
		return 0;
	}
#endif

	return file;
}


static jrd_file* setup_file(Database* dbb, const Firebird::PathName& file_name, int desc)
{
/**************************************
 *
 *	s e t u p _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Set up file and lock blocks for a file.
 *
 **************************************/

/* Allocate file block and copy file name string */

	jrd_file* file = FB_NEW_RPT(*dbb->dbb_permanent, file_name.length() + 1) jrd_file();
	file->fil_desc = desc;
	file->fil_length = file_name.length();
	file->fil_max_page = -1UL;
	MOVE_FAST(file_name.c_str(), file->fil_string, file_name.length());
	file->fil_string[file_name.length()] = '\0';
#ifndef PREAD_PWRITE
	THD_IO_MUTEX_INIT(file->fil_mutex);
#endif

/* If this isn't the primary file, we're done */

	if (dbb->dbb_file)
		return file;

/* Build unique lock string for file and construct lock block */

	struct stat statistics;
	fstat(desc, &statistics);
	UCHAR lock_string[32];
	UCHAR* p = lock_string;

	USHORT l = sizeof(statistics.st_dev);
	memcpy(p, &statistics.st_dev, l);
	p += l;

	l = sizeof(statistics.st_ino);
	memcpy(p, &statistics.st_ino, l);
	p += l;

	l = p - lock_string;
	fb_assert(l <= sizeof(lock_string)); // In case we add more information.

	Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, l) Lock();
	dbb->dbb_lock = lock;
	lock->lck_type = LCK_database;
	lock->lck_owner_handle = LCK_get_owner_handle(NULL, lock->lck_type);
	lock->lck_object = reinterpret_cast<blk*>(dbb);
	lock->lck_length = l;
	lock->lck_dbb = dbb;
	lock->lck_ast = CCH_down_grade_dbb;
	MOVE_FAST(lock_string, lock->lck_key.lck_string, l);

/* Try to get an exclusive lock on database.  If this fails, insist
   on at least a shared lock */

	dbb->dbb_flags |= DBB_exclusive;
	if (!LCK_lock(NULL, lock, LCK_EX, LCK_NO_WAIT)) {
		dbb->dbb_flags &= ~DBB_exclusive;
		thread_db* tdbb = JRD_get_thread_data();
		
		while (!LCK_lock(tdbb, lock, LCK_SW, -1)) {
			tdbb->tdbb_status_vector[0] = 0; // Clean status vector from lock manager error code
			// If we are in a single-threaded maintenance mode then clean up and stop waiting
			SCHAR spare_memory[MIN_PAGE_SIZE * 2];
			SCHAR *header_page_buffer = (SCHAR*) FB_ALIGN((IPTR)spare_memory, MIN_PAGE_SIZE);
		
			try {
				dbb->dbb_file = file;
				PIO_header(dbb, header_page_buffer, MIN_PAGE_SIZE);
				/* Rewind file pointer */
				if (lseek (file->fil_desc, LSEEK_OFFSET_CAST 0, 0) == (off_t)-1)
					ERR_post (isc_io_error,
						isc_arg_string, "lseek",
						isc_arg_cstring, file_name.length(), ERR_cstring (file_name),
						isc_arg_gds, isc_io_read_err,
						isc_arg_unix, errno, isc_arg_end);
				if ((reinterpret_cast<Ods::header_page*>(header_page_buffer)->hdr_flags & Ods::hdr_shutdown_mask) == Ods::hdr_shutdown_single)
					ERR_post(isc_shutdown, isc_arg_cstring, file_name.length(), ERR_cstring(file_name), isc_arg_end);
				dbb->dbb_file = NULL; // Will be set again later by the caller				
			} catch(const std::exception&) {
				delete dbb->dbb_lock;
				dbb->dbb_lock = NULL;
				delete file;
				dbb->dbb_file = NULL; // Will be set again later by the caller
				throw;
			}
		}
	}

	return file;
}


static bool unix_error(
						  TEXT* string,
						  jrd_file* file, ISC_STATUS operation,
						  ISC_STATUS* status_vector)
{
/**************************************
 *
 *	u n i x _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Somebody has noticed a file system error and expects error
 *	to do something about it.  Harumph!
 *
 **************************************/
	ISC_STATUS* status = status_vector;
	if (status) {
		*status++ = isc_arg_gds;
		*status++ = isc_io_error;
		*status++ = isc_arg_string;
		*status++ = (ISC_STATUS) string; // pointer to ISC_STATUS!!!
		*status++ = isc_arg_string;
		*status++ = (ISC_STATUS)(U_IPTR) ERR_string(file->fil_string, file->fil_length);
		*status++ = isc_arg_gds;
		*status++ = operation;
		*status++ = isc_arg_unix;
		*status++ = errno;
		*status++ = isc_arg_end;
		gds__log_status(0, status_vector);
		return false;
	}
	else
		ERR_post(isc_io_error,
				 isc_arg_string, string,
				 isc_arg_string, ERR_string(file->fil_string,
											file->fil_length),
				 isc_arg_gds,
				 operation, isc_arg_unix, errno, isc_arg_end);


    // Added a false for final return - which seems to be the answer,
    // but is better than what it was which was nothing ie random 
    // Most usages within here want it to return a failure.
    // MOD 01-July-2002

    return false;
}

#if defined PREAD_PWRITE && !(defined HAVE_PREAD && defined HAVE_PWRITE)

/* pread() and pwrite() behave like read() and write() except that they
   take an additional 'offset' argument. The I/O takes place at the specified
   'offset' from the beginning of the file and does not affect the offset
   associated with the file descriptor.
   This is done in order to allow more than one thread to operate on
   individual records within the same file simultaneously. This is
   called Positioned I/O. Since positioned I/O is not currently directly
   available through the POSIX interfaces, it has been implemented
   using the POSIX asynchronous I/O calls.

   NOTE: pread() and pwrite() are defined in UNIX International system
         interface and are a part of POSIX systems.
*/

static SLONG pread(int fd, SCHAR * buf, SLONG nbytes, SLONG offset)
/**************************************
 *
 *	p r e a d
 *
 **************************************
 *
 * Functional description
 *
 *   This function uses Asynchronous I/O calls to implement
 *   positioned read from a given offset
 **************************************/
{
	struct aiocb io;
	io.aio_fildes = fd;
	io.aio_offset = offset;
	io.aio_buf = buf;
	io.aio_nbytes = nbytes;
	io.aio_reqprio = 0;
	io.aio_sigevent.sigev_notify = SIGEV_NONE;
	int err = aio_read(&io);		/* atomically reads at offset */
	if (err != 0)
		return (err);			/* errno is set */

	struct aiocb *list[1];
	list[0] = &io;
	err = aio_suspend(list, 1, NULL);	/* wait for I/O to complete */
	if (err != 0)
		return (err);			/* errno is set */
	return (aio_return(&io));	/* return I/O status */
}

static SLONG pwrite(int fd, SCHAR * buf, SLONG nbytes, SLONG offset)
/**************************************
 *
 *	p w r i t e
 *
 **************************************
 *
 * Functional description
 *
 *   This function uses Asynchronous I/O calls to implement
 *   positioned write from a given offset
 **************************************/
{
	struct aiocb io;
	io.aio_fildes = fd;
	io.aio_offset = offset;
	io.aio_buf = buf;
	io.aio_nbytes = nbytes;
	io.aio_reqprio = 0;
	io.aio_sigevent.sigev_notify = SIGEV_NONE;
	int err = aio_write(&io);		/* atomically reads at offset */
	if (err != 0)
		return (err);			/* errno is set */

	struct aiocb *list[1];
	list[0] = &io;
	err = aio_suspend(list, 1, NULL);	/* wait for I/O to complete */
	if (err != 0)
		return (err);			/* errno is set */
	return (aio_return(&io));	/* return I/O status */
}

#endif /* PREAD_PWRITE && !(HAVE_PREAD && HAVE_PWRITE)*/


#ifdef SUPPORT_RAW_DEVICES
int PIO_unlink (const Firebird::PathName& file_name)
{
/**************************************
 *
 *	P I O _ u n l i n k
 *
 **************************************
 *
 * Functional description
 *	Delete a database file.
 *
 **************************************/

	if (PIO_on_raw_device(file_name))
		return raw_devices_unlink_database(file_name);
	else
		return unlink(file_name.c_str());
}


bool PIO_on_raw_device(
	const Firebird::PathName& file_name)
{
/**************************************
 *
 *	P I O _ o n _ r a w _ d e v i c e
 *
 **************************************
 *
 * Functional description
 *	Checks if the supplied file name is a special file
 *
 **************************************/
	struct stat s;

	return (stat(file_name.c_str(), &s) == 0
			&& (S_ISCHR(s.st_mode) || S_ISBLK(s.st_mode)));
}


static bool raw_devices_validate_database (
	int desc,
	const Firebird::PathName& file_name)
{
/**************************************
 *
 *	raw_devices_validate_database
 *
 **************************************
 *
 * Functional description
 *	Checks if the special file contains a valid database
 *
 **************************************/
	char header[MIN_PAGE_SIZE];
	const Ods::header_page* hp = (Ods::header_page*)header;
	bool retval = false;

	/* Read in database header. Code lifted from PIO_header. */
	if (desc == -1)
		ERR_post (isc_io_error,
					isc_arg_string, "raw_devices_validate_database",
					isc_arg_string, file_name.length(), ERR_cstring (file_name),
					isc_arg_gds, isc_io_read_err,
					isc_arg_unix, errno, isc_arg_end);

	for (int i = 0; i < IO_RETRY; i++)
	{
		if (lseek (desc, LSEEK_OFFSET_CAST 0, 0) == (off_t) -1)
			ERR_post (isc_io_error,
						isc_arg_string, "lseek",
						isc_arg_string, file_name.length(), ERR_cstring (file_name),
						isc_arg_gds, isc_io_read_err,
						isc_arg_unix, errno, isc_arg_end);
		const ssize_t bytes = read (desc, header, sizeof(header));
		if (bytes == sizeof(header))
			goto read_finished;
		if (bytes == -1 && !SYSCALL_INTERRUPTED(errno))
			ERR_post (isc_io_error,
						isc_arg_string, "read",
						isc_arg_string, file_name.length(), ERR_cstring (file_name),
						isc_arg_gds, isc_io_read_err,
						isc_arg_unix, errno, isc_arg_end);
	}

	ERR_post (isc_io_error,
				isc_arg_string, "read_retry",
				isc_arg_string, file_name.length(), ERR_cstring (file_name),
				isc_arg_gds, isc_io_read_err,
				isc_arg_unix, errno, isc_arg_end);

  read_finished:
	/* Rewind file pointer */
	if (lseek (desc, LSEEK_OFFSET_CAST 0, 0) == (off_t)-1)
		ERR_post (isc_io_error,
					isc_arg_string, "lseek",
					isc_arg_string, file_name.length(), ERR_cstring (file_name),
					isc_arg_gds, isc_io_read_err,
					isc_arg_unix, errno, isc_arg_end);

	/* Validate database header. Code lifted from PAG_header. */
	if (hp->hdr_header.pag_type != pag_header /*|| hp->hdr_sequence*/)
		goto quit;

	if (!Ods::isSupported(hp->hdr_ods_version, hp->hdr_ods_minor))
		goto quit;

	if (hp->hdr_page_size < MIN_PAGE_SIZE || hp->hdr_page_size > MAX_PAGE_SIZE)
		goto quit;

	/* At this point we think we have identified a database on the device.
 	 * PAG_header will validate the entire structure later.
 	 */
	retval = true;

  quit:
#ifdef DEV_BUILD
	gds__log ("raw_devices_validate_database: %s -> %s%s\n",
		 file_name.c_str(),
		 retval ? "true" : "false",
		 retval && hp->hdr_sequence != 0 ? " (continuation file)" : "");
#endif
	return retval;
}


static int raw_devices_unlink_database (
	const Firebird::PathName& file_name)
{
	char header[MIN_PAGE_SIZE];
	int desc = -1, i;

	for (i = 0; i < IO_RETRY; i++)
	{
		if ((desc = open (file_name.c_str(), O_RDWR | O_BINARY)) != -1)
			break;
		if (!SYSCALL_INTERRUPTED(errno))
			ERR_post (isc_io_error,
						isc_arg_string, "open",
						isc_arg_string, file_name.length(), ERR_cstring (file_name),
						isc_arg_gds, isc_io_open_err,
						isc_arg_unix, errno, isc_arg_end);
	}

	memset(header, 0xa5, sizeof(header));

	for (i = 0; i < IO_RETRY; i++)
	{
		const ssize_t bytes = write (desc, header, sizeof(header));
		if (bytes == sizeof(header))
			break;
		if (bytes == -1 && SYSCALL_INTERRUPTED(errno))
			continue;
		ERR_post (isc_io_error,
			isc_arg_string, "write",
			isc_arg_string, file_name.length(), ERR_cstring (file_name),
			isc_arg_gds, isc_io_write_err,
			isc_arg_unix, errno, isc_arg_end);
	}

	//if (desc != -1) perhaps it's better to check this???
		(void)close(desc);

#if DEV_BUILD
	gds__log ("raw_devices_unlink_database: %s -> %s\n",
				file_name.c_str(), i < IO_RETRY ? "true" : "false");
#endif

	return 0;
}
#endif // SUPPORT_RAW_DEVICES
