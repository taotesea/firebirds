/*
 *	PROGRAM:	JRD access method
 *	MODULE:		thd.h
 *	DESCRIPTION:	Thread support definitions
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
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */
/*
$Id: thd.h,v 1.36 2004-08-27 17:39:34 skidder Exp $
*/

#ifndef JRD_THD_H
#define JRD_THD_H

#include "firebird.h"
#include "../jrd/isc.h"
#include "../common/classes/locks.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/alloc.h"

// compatibility definitions
enum	WLCK_type {WLCK_read = 1, WLCK_write = 2};
int		THD_wlck_lock(struct wlck_t*, enum WLCK_type);
int		THD_wlck_unlock(struct wlck_t*);

inline int	THD_mutex_lock(Firebird::Mutex* m) {
	m->enter();
	return 0;
}

inline int	THD_mutex_unlock(Firebird::Mutex* m) {
	m->leave();
	return 0;
}

extern	Firebird::Mutex ib_mutex;
inline int	THD_mutex_lock_global(void) {
	ib_mutex.enter();
	return 0;
}
inline int	THD_mutex_unlock_global(void) {
	ib_mutex.leave();
	return 0;
}

// recursive mutex
#ifdef SUPERSERVER
int		THD_rec_mutex_destroy(struct rec_mutx_t*);
int		THD_rec_mutex_init(struct rec_mutx_t*);
int		THD_rec_mutex_lock(struct rec_mutx_t*);
int		THD_rec_mutex_unlock(struct rec_mutx_t*);
#endif

// thread run-ability control
int		THD_resume(THD_T);
void	THD_sleep(ULONG);
int		THD_suspend(THD_T);
void	THD_yield(void);

/* Thread priorities (may be ignored) */

const int THREAD_high			= 1;
const int THREAD_medium_high	= 2;
const int THREAD_medium			= 3;
const int THREAD_medium_low		= 4;
const int THREAD_low			= 5;
const int THREAD_critical		= 6;

/* Thread option flags */

const int THREAD_ast		= 1;	/* Thread can/should run at ast level */
const int THREAD_blast		= 2;	/* Blow away thread during exit handler */
const int THREAD_wait		= 4;	/* Somebody will wait for thread exit */

/* Thread quanta */

const int QUANTUM			= 100;	/* Default quantum */
const int SWEEP_QUANTUM		= 10;	/* Make sweeps less disruptive */


/* Thread specific data */

#if defined(WIN_NT)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN unsigned int
#define THREAD_ENTRY_CALL __stdcall
#elif defined(USE_POSIX_THREADS)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN void*
#define THREAD_ENTRY_CALL
#elif defined(SOLARIS)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN void *
#define THREAD_ENTRY_CALL
#else
// Define correct types for other platforms
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN int
#define THREAD_ENTRY_CALL
#endif

#define THREAD_ENTRY_DECLARE THREAD_ENTRY_RETURN THREAD_ENTRY_CALL

// BRS 01/07/2004
// Hack due to a bug in mingw.
// The definition inside the thdd class should be replaced with the following one.
typedef THREAD_ENTRY_DECLARE ThreadEntryPoint(THREAD_ENTRY_PARAM);

class thdd
{
public:
	thdd*		thdd_prior_context;
	ULONG		thdd_type;	/* what kind of thread context this is */
//public:
//	typedef THREAD_ENTRY_DECLARE EntryPoint(THREAD_ENTRY_PARAM);

public:
	static void		start(ThreadEntryPoint* routine, 
						  void* arg, 
						  int priority_arg, 
						  int flags, 
						  void* thd_id);
	static void		init(void) {}
	static void		cleanup(void) {}
	static thdd*	getSpecific(void);
	void			putSpecific();
	static void		restoreSpecific(void);
	static FB_THREAD_ID getId(void);
	static void		getSpecificData(void** t_data);
	static void		putSpecificData(void* t_data);

private:
	MemoryPool*	thdd_pool;

public:
    thdd() : thdd_prior_context(NULL), thdd_type(0), thdd_pool(NULL) {}

	void makeDefaultPool()
	{
		thdd* previous = getSpecific();
		if (previous)
		{
			thdd_pool = &previous->getPool();
		}
		if (! thdd_pool)
		{
			thdd_pool = getDefaultMemoryPool();
		}
	}

	void setPool(MemoryPool* p)
	{
		if (p)
		{
			thdd_pool = p;
		}
		else
		{
			makeDefaultPool();
		}
	}

	MemoryPool& getPool()
	{
		return *thdd_pool;
	}

	static MemoryPool& getDefaultPool()
	{
		return getSpecific()->getPool();
	}
};

/* Thread structure types */

const USHORT THDD_TYPE_TGBL	= 1;		/* used by backup/restore */
const USHORT THDD_TYPE_TSQL	= 2;		/* used by DSQL */
const USHORT THDD_TYPE_TDBB	= 3;		/* used in engine */
const USHORT THDD_TYPE_TRDB	= 4;		/* used in remote interface */
const USHORT THDD_TYPE_TDBA	= 5;		/* used in DBA utility */
const USHORT THDD_TYPE_TIDB	= 6;		/* used by interprocess server */
const USHORT THDD_TYPE_TALICE	= 7;		/* used by gfix */
const USHORT THDD_TYPE_TSEC	= 8;		/* used by gsec */

/* General purpose in use object structure */

struct iuo {
	struct iuo *iuo_next;
	void *iuo_object[10];
	USHORT iuo_in_use_count;
};

typedef iuo *IUO;

/* Mutex structure */

typedef Firebird::Mutex MUTX_T;
typedef Firebird::Mutex* MUTX;

/* Recursive mutex structure */
struct rec_mutx_t {
	MUTX_T rec_mutx_mtx[1];
	FB_THREAD_ID rec_mutx_id;
	SLONG rec_mutx_count;
};

typedef rec_mutx_t REC_MUTX_T;
typedef rec_mutx_t *REC_MUTX;

/* Read/write lock structure */

struct wlck_t {
	Firebird::RWLock rwLock;
	WLCK_type type;
};

typedef struct wlck_t WLCK_T;
typedef struct wlck_t* WLCK;

#ifdef V4_THREADING
#define V4_MUTEX_LOCK(mutx)		THD_mutex_lock (mutx)
#define V4_MUTEX_UNLOCK(mutx)		THD_mutex_unlock (mutx)
#define V4_GLOBAL_MUTEX_LOCK		THD_mutex_lock_global()
#define V4_GLOBAL_MUTEX_UNLOCK		THD_mutex_unlock_global()
#define V4_RW_LOCK_LOCK(wlck,type)	THD_wlck_lock (wlck, type)
#define V4_RW_LOCK_UNLOCK(wlck)		THD_wlck_unlock (wlck)
#endif

#ifdef ANY_THREADING
#define THD_INIT
#define THD_GLOBAL_MUTEX_LOCK		THD_mutex_lock_global()
#define THD_GLOBAL_MUTEX_UNLOCK		THD_mutex_unlock_global()
#define THD_MUTEX_LOCK(mutx)		THD_mutex_lock (mutx)
#define THD_MUTEX_UNLOCK(mutx)		THD_mutex_unlock (mutx)
#else
#define THD_INIT
#define THD_GLOBAL_MUTEX_LOCK
#define THD_GLOBAL_MUTEX_UNLOCK
#define THD_MUTEX_LOCK(mutx)
#define THD_MUTEX_UNLOCK(mutx)
#endif

extern "C" {
	int		API_ROUTINE gds__thread_start(ThreadEntryPoint*, void*, int, int,
										 void*);
}

#endif /* JRD_THD_H */
