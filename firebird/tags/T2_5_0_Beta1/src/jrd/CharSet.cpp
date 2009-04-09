/*
 *      PROGRAM:        JRD Intl
 *      MODULE:         CharSet.cpp
 *      DESCRIPTION:    International text support routines
 *
 * copyright (c) 1992, 1993 by Borland International
 */

/*************  history ************
*
*       COMPONENT: JRD  MODULE: INTL.CPP
*       generated by Marion V2.5     2/6/90
*       from dev              db        on 4-JAN-1995
*****************************************************************
*
*       PR	2002-06-02 Added ugly c hack in
*       intl_back_compat_alloc_func_lookup.
*       When someone has time we need to change the references to
*       return (void*) function to something more C++ like
*
*       42 4711 3 11 17  tamlin   2001
*       Added silly numbers before my name, and converted it to C++.
*
*       18850   daves   4-JAN-1995
*       Fix gds__alloc usage
*
*       18837   deej    31-DEC-1994
*       fixing up HARBOR_MERGE
*
*       18821   deej    27-DEC-1994
*       HARBOR MERGE
*
*       18789   jdavid  19-DEC-1994
*       Cast some functions
*
*       17508   jdavid  15-JUL-1994
*       Bring it up to date
*
*       17500   daves   13-JUL-1994
*       Bug 6645: Different calculation of partial keys
*
*       17202   katz    24-MAY-1994
*       PC_PLATFORM requires the .dll extension
*
*       17191   katz    23-MAY-1994
*       OS/2 requires the .dll extension
*
*       17180   katz    23-MAY-1994
*       Define location of DLL on OS/2
*
*       17149   katz    20-MAY-1994
*       In JRD, isc_arg_number arguments are SLONG's not int's
*
*       16633   daves   19-APR-1994
*       Bug 6202: International licensing uses INTERNATIONAL product code
*
*       16555   katz    17-APR-1994
*       The last argument of calls to ERR_post should be 0
*
*       16521   katz    14-APR-1994
*       Borland C needs a decorated symbol to lookup
*
*       16403   daves   8-APR-1994
*       Bug 6441: Emit an error whenever transliteration from ttype_binary attempted
*
*       16141   katz    28-MAR-1994
*       Don't declare return value from ISC_lookup_entrypoint as API_ROUTINE
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 * 2006.10.10 Adriano dos Santos Fernandes - refactored from intl.cpp
 *
*/

#include "firebird.h"
#include "../jrd/intl_classes.h"
#include "../common/classes/Aligner.h"

using namespace Jrd;
using namespace Firebird;


namespace
{

class FixedWidthCharSet : public CharSet
{
public:
	FixedWidthCharSet(USHORT _id, charset* _cs) : CharSet(_id, _cs) {}

	virtual ULONG length(ULONG srcLen, const UCHAR* src, bool countTrailingSpaces) const;
	virtual ULONG substring(ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst, ULONG startPos, ULONG len) const;
};

class MultiByteCharSet : public CharSet
{
public:
	MultiByteCharSet(USHORT _id, charset* _cs) : CharSet(_id, _cs) {}

	virtual ULONG length(ULONG srcLen, const UCHAR* src, bool countTrailingSpaces) const;
	virtual ULONG substring(ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst, ULONG startPos, ULONG len) const;
};

}	// namespace


//-------------


ULONG FixedWidthCharSet::length(ULONG srcLen, const UCHAR* src, bool countTrailingSpaces) const
{
	if (!countTrailingSpaces)
		srcLen = removeTrailingSpaces(srcLen, src);

	if (getStruct()->charset_fn_length)
		return getStruct()->charset_fn_length(getStruct(), srcLen, src);

	return srcLen / minBytesPerChar();
}


ULONG FixedWidthCharSet::substring(ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst, ULONG startPos, ULONG len) const
{
	ULONG result;

	if (getStruct()->charset_fn_substring)
		result = getStruct()->charset_fn_substring(getStruct(), srcLen, src, dstLen, dst, startPos, len);
	else
	{
		fb_assert(src != NULL && dst != NULL);

		result = MIN(srcLen / minBytesPerChar() - startPos, len) * minBytesPerChar();

		if (dstLen < result)
			result = INTL_BAD_STR_LENGTH;
		else if (startPos * minBytesPerChar() > srcLen)
			result = 0;
		else
			memcpy(dst, src + startPos * minBytesPerChar(), result);
	}

	if (result == INTL_BAD_STR_LENGTH)
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_string_truncation));

	return result;
}


//-------------


ULONG MultiByteCharSet::length(ULONG srcLen, const UCHAR* src, bool countTrailingSpaces) const
{
	if (!countTrailingSpaces)
		srcLen = removeTrailingSpaces(srcLen, src);

	if (getStruct()->charset_fn_length)
		return getStruct()->charset_fn_length(getStruct(), srcLen, src);

	ULONG len = getConvToUnicode().convertLength(srcLen);

	// convert to UTF16
	HalfStaticArray<USHORT, BUFFER_SMALL / sizeof(USHORT)> str;
	len = getConvToUnicode().convert(srcLen, src, len,
					str.getBuffer(len / sizeof(USHORT)));

	// calculate length of UTF16
	return UnicodeUtil::utf16Length(len, str.begin());
}


ULONG MultiByteCharSet::substring(ULONG srcLen, const UCHAR* src, ULONG dstLen, UCHAR* dst, ULONG startPos, ULONG len) const
{
	ULONG result;

	if (getStruct()->charset_fn_substring)
		result = getStruct()->charset_fn_substring(getStruct(), srcLen, src, dstLen, dst, startPos, len);
	else
	{
		fb_assert(src != NULL && dst != NULL);

		if (len == 0 || startPos >= srcLen)
			return 0;

		// convert to UTF16
		HalfStaticArray<UCHAR, BUFFER_SMALL> str;
		ULONG unilength = getConvToUnicode().convertLength(srcLen);

		// ASF: We should pass badInputPos to convert for it not throw in the case
		// of irrelevant (for substring) invalid bytes in the end of the string.
		// This can occur with substring of blobs.
		// We'll then assume the string is already well formed, because verifying
		// this may be costly.
		ULONG badInputPos;
		unilength = getConvToUnicode().convert(srcLen, src, unilength,
			OutAligner<USHORT>(str.getBuffer(unilength), unilength), &badInputPos);

		// generate substring of UTF16
		HalfStaticArray<UCHAR, BUFFER_SMALL> substr;
		unilength = UnicodeUtil::utf16Substring(unilength, Aligner<USHORT>(str.begin(), unilength),
			unilength, OutAligner<USHORT>(substr.getBuffer(unilength), unilength), startPos, len);

		// convert generated substring to original charset
		result = getConvFromUnicode().convert(unilength, substr.begin(), dstLen, dst);
	}

	if (result == INTL_BAD_STR_LENGTH)
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_string_truncation));

	return result;
}


//-------------


namespace Jrd {


CharSet* CharSet::createInstance(MemoryPool& pool, USHORT id, charset* cs)
{
	if (cs->charset_min_bytes_per_char != cs->charset_max_bytes_per_char)
		return FB_NEW(pool) MultiByteCharSet(id, cs);

	return FB_NEW(pool) FixedWidthCharSet(id, cs);
}


}	// namespace Jrd
