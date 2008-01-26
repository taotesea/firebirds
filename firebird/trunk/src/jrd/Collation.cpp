/*
 *      PROGRAM:        JRD Intl
 *      MODULE:         Collation.cpp
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
#include "gen/iberror.h"
#include "../jrd/jrd.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_string.h"
#include "../jrd/intl_classes.h"
#include "../jrd/lck_proto.h"
#include "../jrd/intl_classes.h"
#include "../jrd/TextType.h"

using namespace Jrd;

#if defined _MSC_VER && _MSC_VER <= 1200
#include "../jrd/SimilarToMatcherVC6.h"
#else
#include "../jrd/SimilarToMatcher.h"
#endif


namespace {

// constants used in matches and sleuth
const int CHAR_GDML_MATCH_ONE	= TextType::CHAR_QUESTION_MARK;
const int CHAR_GDML_MATCH_ANY	= TextType::CHAR_ASTERISK;
const int CHAR_GDML_QUOTE		= TextType::CHAR_AT;
const int CHAR_GDML_NOT			= TextType::CHAR_TILDE;
const int CHAR_GDML_RANGE		= TextType::CHAR_MINUS;
const int CHAR_GDML_CLASS_START	= TextType::CHAR_OPEN_BRACKET;
const int CHAR_GDML_CLASS_END	= TextType::CHAR_CLOSE_BRACKET;
const int CHAR_GDML_SUBSTITUTE	= TextType::CHAR_EQUAL;
const int CHAR_GDML_FLAG_SET	= TextType::CHAR_PLUS;
const int CHAR_GDML_FLAG_CLEAR	= TextType::CHAR_MINUS;
const int CHAR_GDML_COMMA		= TextType::CHAR_COMMA;
const int CHAR_GDML_LPAREN		= TextType::CHAR_OPEN_PAREN;
const int CHAR_GDML_RPAREN		= TextType::CHAR_CLOSE_PAREN;

static const UCHAR SLEUTH_SPECIAL[128] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0,	// $%*+- (dollar, percent, star, plus, minus)
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,	// ?     (question)
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// @     (at-sign)
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,	// [     (open square)
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0	// ~     (tilde)
};

// Below are templates for functions used in Collation implementation

class NullStrConverter
{
public:
	NullStrConverter(MemoryPool& pool, const TextType* obj, const UCHAR* str, SLONG len)
	{
	}
};

template <typename PrevConverter>
class UpcaseConverter : public PrevConverter
{
public:
	UpcaseConverter(MemoryPool& pool, TextType* obj, const UCHAR*& str, SLONG& len)
		: PrevConverter(pool, obj, str, len)
	{
		if (len > (int) sizeof(tempBuffer))
			out_str = FB_NEW(pool) UCHAR[len];
		else
			out_str = tempBuffer;
		obj->str_to_upper(len, str, len, out_str);
		str = out_str;
	}

	~UpcaseConverter()
	{
		if (out_str != tempBuffer)
			delete[] out_str;
	}

private:
	UCHAR tempBuffer[100];
	UCHAR* out_str;
};

template <typename PrevConverter>
class CanonicalConverter : public PrevConverter
{
public:
	CanonicalConverter(MemoryPool& pool, TextType* obj, const UCHAR*& str, SLONG& len)
		: PrevConverter(pool, obj, str, len)
	{
		const SLONG out_len = len / obj->getCharSet()->minBytesPerChar() * obj->getCanonicalWidth();

		if (out_len > (int) sizeof(tempBuffer))
			out_str = FB_NEW(pool) UCHAR[out_len];
		else
			out_str = tempBuffer;

		if (str)
		{
			len = obj->canonical(len, str, out_len, out_str) * obj->getCanonicalWidth();
			str = out_str;
		}
		else
			len = 0;
	}

	~CanonicalConverter()
	{
		if (out_str != tempBuffer)
			delete[] out_str;
	}

private:
	UCHAR tempBuffer[100];
	UCHAR* out_str;
};

template <typename StrConverter, typename CharType>
class LikeMatcher : public PatternMatcher
{
public:
	LikeMatcher(MemoryPool& pool, TextType* ttype, const CharType* str, SLONG str_len,
			CharType escape, bool use_escape, CharType sql_match_any, CharType sql_match_one)
		: PatternMatcher(pool, ttype),
		  evaluator(pool, str, str_len, escape, use_escape, sql_match_any, sql_match_one)
	{
	}

	void reset()
	{
		evaluator.reset();
	}

	bool result()
	{
		return evaluator.getResult();
	}

	bool process(const UCHAR* str, SLONG length)
	{
		StrConverter cvt(pool, textType, str, length);
		fb_assert(length % sizeof(CharType) == 0);
		return evaluator.processNextChunk(
			reinterpret_cast<const CharType*>(str), length / sizeof(CharType));
	}

	static LikeMatcher* create(MemoryPool& pool, TextType* ttype, const UCHAR* str,
		SLONG length, const UCHAR* escape, SLONG escape_length,
		const UCHAR* sql_match_any, SLONG match_any_length,
		const UCHAR* sql_match_one, SLONG match_one_length)
	{
		StrConverter cvt(pool, ttype, str, length),
					 cvt_escape(pool, ttype, escape, escape_length),
					 cvt_match_any(pool, ttype, sql_match_any, match_any_length),
					 cvt_match_one(pool, ttype, sql_match_one, match_one_length);

		fb_assert(length % sizeof(CharType) == 0);
		return FB_NEW(pool) LikeMatcher(pool, ttype,
			reinterpret_cast<const CharType*>(str), length / sizeof(CharType),
			(escape ? *reinterpret_cast<const CharType*>(escape) : 0), escape_length != 0,
			*reinterpret_cast<const CharType*>(sql_match_any),
			*reinterpret_cast<const CharType*>(sql_match_one));
	}

	static bool evaluate(MemoryPool& pool, TextType* ttype, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length,
		const UCHAR* sql_match_any, SLONG match_any_length,
		const UCHAR* sql_match_one, SLONG match_one_length)
	{
		StrConverter cvt1(pool, ttype, p, pl),
					 cvt2(pool, ttype, s, sl),
					 cvt_escape(pool, ttype, escape, escape_length),
					 cvt_match_any(pool, ttype, sql_match_any, match_any_length),
					 cvt_match_one(pool, ttype, sql_match_one, match_one_length);

		fb_assert(pl % sizeof(CharType) == 0);
		fb_assert(sl % sizeof(CharType) == 0);
		Firebird::LikeEvaluator<CharType> evaluator(pool,
			reinterpret_cast<const CharType*>(p), pl / sizeof(CharType),
			(escape ? *reinterpret_cast<const CharType*>(escape) : 0), escape_length != 0,
			*reinterpret_cast<const CharType*>(sql_match_any),
			*reinterpret_cast<const CharType*>(sql_match_one));
		evaluator.processNextChunk(reinterpret_cast<const CharType*>(s), sl / sizeof(CharType));
		return evaluator.getResult();
	}

private:
	Firebird::LikeEvaluator<CharType> evaluator;
};

template <typename StrConverter, typename CharType>
class StartsMatcher : public PatternMatcher
{
public:
	StartsMatcher(MemoryPool& pool, TextType* ttype, const CharType* str, SLONG str_len)
		: PatternMatcher(pool, ttype),
		  evaluator(pool, str, str_len)
	{
	}

	void reset()
	{
		evaluator.reset();
	}

	bool result()
	{
		return evaluator.getResult();
	}

	bool process(const UCHAR* str, SLONG length)
	{
		StrConverter cvt(pool, textType, str, length);
		fb_assert(length % sizeof(CharType) == 0);
		return evaluator.processNextChunk(
			reinterpret_cast<const CharType*>(str), length / sizeof(CharType));
	}

	static StartsMatcher* create(MemoryPool& pool, TextType* ttype,
		const UCHAR* str, SLONG length)
	{
		StrConverter cvt(pool, ttype, str, length);
		fb_assert(length % sizeof(CharType) == 0);
		return FB_NEW(pool) StartsMatcher(pool, ttype,
			reinterpret_cast<const CharType*>(str), length / sizeof(CharType));
	}

	static bool evaluate(MemoryPool& pool, TextType* ttype, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl)
	{
		StrConverter cvt1(pool, ttype, p, pl);
		StrConverter cvt2(pool, ttype, s, sl);
		fb_assert(pl % sizeof(CharType) == 0);
		fb_assert(sl % sizeof(CharType) == 0);
		Firebird::StartsEvaluator<CharType> evaluator(pool,
			reinterpret_cast<const CharType*>(p), pl / sizeof(CharType));
		evaluator.processNextChunk(reinterpret_cast<const CharType*>(s), sl / sizeof(CharType));
		return evaluator.getResult();
	}

private:
	Firebird::StartsEvaluator<CharType> evaluator;
};

template <typename StrConverter, typename CharType>
class ContainsMatcher : public PatternMatcher
{
public:
	ContainsMatcher(MemoryPool& pool, TextType* ttype, const CharType* str, SLONG str_len)
		: PatternMatcher(pool, ttype),
		  evaluator(pool, str, str_len)
	{
	}

	void reset()
	{
		evaluator.reset();
	}

	bool result()
	{
		return evaluator.getResult();
	}

	bool process(const UCHAR* str, SLONG length)
	{
		StrConverter cvt(pool, textType, str, length);
		fb_assert(length % sizeof(CharType) == 0);
		return evaluator.processNextChunk(
			reinterpret_cast<const CharType*>(str), length / sizeof(CharType));
	}

	static ContainsMatcher* create(MemoryPool& pool, TextType* ttype,
		const UCHAR* str, SLONG length)
	{
		StrConverter cvt(pool, ttype, str, length);
		fb_assert(length % sizeof(CharType) == 0);
		return FB_NEW(pool) ContainsMatcher(pool, ttype,
			reinterpret_cast<const CharType*>(str), length / sizeof(CharType));
	}

	static bool evaluate(MemoryPool& pool, TextType* ttype, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl)
	{
		StrConverter cvt1(pool, ttype, p, pl);
		StrConverter cvt2(pool, ttype, s, sl);
		fb_assert(pl % sizeof(CharType) == 0);
		fb_assert(sl % sizeof(CharType) == 0);
		Firebird::ContainsEvaluator<CharType> evaluator(pool,
			reinterpret_cast<const CharType*>(p), pl / sizeof(CharType));
		evaluator.processNextChunk(reinterpret_cast<const CharType*>(s), sl / sizeof(CharType));
		return evaluator.getResult();
	}

private:
	Firebird::ContainsEvaluator<CharType> evaluator;
};

template <typename StrConverter, typename CharType>
class MatchesMatcher
{
public:
	static bool evaluate(MemoryPool& pool, TextType* ttype,
		const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl)
	{
		StrConverter cvt1(pool, ttype, p, pl);
		StrConverter cvt2(pool, ttype, s, sl);
		fb_assert(pl % sizeof(CharType) == 0);
		fb_assert(sl % sizeof(CharType) == 0);
		return matches(pool, ttype, reinterpret_cast<const CharType*>(s), sl,
						   reinterpret_cast<const CharType*>(p), pl);
	}

private:
	 // Return true if a string (p1, l1) matches a given pattern (p2, l2).
	 // The character '?' in the pattern may match any single character
	 // in the the string, and the character '*' may match any sequence
	 // of characters.
	 //
	 // Wide SCHAR version operates on short-based buffer,
	 // instead of SCHAR-based.
	 //
	 // Matches is not a case-sensitive operation, thus it has no
	 // 8-bit international impact.
	static bool matches(MemoryPool& pool, Jrd::TextType* obj, const CharType* p1,
		SLONG l1_bytes, const CharType* p2, SLONG l2_bytes)
	{
		fb_assert(p1 != NULL);
		fb_assert(p2 != NULL);
		fb_assert((l1_bytes % sizeof(CharType)) == 0);
		fb_assert((l2_bytes % sizeof(CharType)) == 0);
		fb_assert((obj->getCanonicalWidth() == sizeof(CharType)));

		SLONG l1 = l1_bytes / sizeof(CharType);
		SLONG l2 = l2_bytes / sizeof(CharType);

		while (l2-- > 0) {
			const CharType c = *p2++;
			if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ANY)) {
				while ((l2 > 0) && (*p2 == *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ANY))) {
					l2--;
					p2++;
				}
				if (l2 == 0)
					return true;
				while (l1)
				{
					if (matches(pool, obj, p1++, l1-- * sizeof(CharType),
							p2, l2 * sizeof(CharType)))
					{
						return true;
					}
				}
				return false;
			}
			if (l1-- == 0)
				return false;
			if (c != *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ONE) && c != *p1)
				return false;
			p1++;
		}

		return !l1;
	}
};

template <typename StrConverter, typename CharType>
class SleuthMatcher
{
public:
	// Evaluate the "sleuth" search operator.
	// Turn the (pointer, byte length) input parameters into
	// (pointer, end_pointer) for use in aux function
	static bool check(MemoryPool& pool, TextType* ttype, USHORT flags,
		const UCHAR* search, SLONG search_len,
		const UCHAR* match, SLONG match_len)
	{
		StrConverter cvt1(pool, ttype, search, search_len);//, cvt2(pool, ttype, match, match_len);

		fb_assert((match_len % sizeof(CharType)) == 0);
		fb_assert((search_len % sizeof(CharType)) == 0);
		fb_assert(ttype->getCanonicalWidth() == sizeof(CharType));

		const CharType* const end_match = reinterpret_cast<const CharType*>(match) +
			(match_len / sizeof(CharType));
		const CharType* const end_search = reinterpret_cast<const CharType*>(search) +
			(search_len / sizeof(CharType));

		return aux(ttype, flags, reinterpret_cast<const CharType*>(search),
			end_search, reinterpret_cast<const CharType*>(match), end_match);
	}

	static ULONG merge(MemoryPool& pool, TextType* ttype,
		const UCHAR* match, SLONG match_bytes,
		const UCHAR* control, SLONG control_bytes,
		UCHAR* combined, SLONG combined_bytes)
	{
		StrConverter cvt1(pool, ttype, match, match_bytes);
		StrConverter cvt2(pool, ttype, control, control_bytes);
		fb_assert(match_bytes % sizeof(CharType) == 0);
		fb_assert(control_bytes % sizeof(CharType) == 0);
		return actualMerge(pool, ttype,
						   reinterpret_cast<const CharType*>(match), match_bytes,
						   reinterpret_cast<const CharType*>(control), control_bytes,
						   reinterpret_cast<CharType*>(combined), combined_bytes);
	}

private:
	// Evaluate the "sleuth" search operator.
	static bool aux(Jrd::TextType* obj, USHORT flags,
		const CharType* search, const CharType* end_search,
		const CharType* match, const CharType* end_match)
	{
		fb_assert(search != NULL);
		fb_assert(end_search != NULL);
		fb_assert(match != NULL);
		fb_assert(end_match != NULL);
		fb_assert(search <= end_search);
		fb_assert(match <= end_match);
		fb_assert(obj->getCanonicalWidth() == sizeof(CharType));

		while (match < end_match) {
			CharType c = *match++;
			if ((c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE) && (c = *match++)) ||
				((((size_t) c) < FB_NELEM(SLEUTH_SPECIAL)) && !SLEUTH_SPECIAL[c]))
			{
				if (match >= end_match || *match != *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ANY)) {
					if (search >= end_search)
						return false;
					const CharType d = *search++;
					if (c != d)
						return false;
				}
				else {
					++match;
					for (;;)
					{
						if (aux(obj, flags, search, end_search, match, end_match))
							return true;
							
						if (search < end_search)
						{
							const CharType d = *search++;
							if (c != d)
								return false;
						}
						else
							return false;
					}
				}
			}
			else if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ONE))
			{
				if (match >= end_match || *match != *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ANY))
				{
					if (search >= end_search)
						return false;
						
					search++;
				}
				else {
					if (++match >= end_match)
						return true;
						
					for (;;)
					{
						if (aux(obj, flags, search, end_search, match, end_match))
							return true;
							
						if (++search >= end_search)
							return false;
					}
				}
			}
			else if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_CLASS_START))
			{
				const CharType* const char_class = match;
				while (*match++ != *(CharType*) obj->getCanonicalChar(CHAR_GDML_CLASS_END)) {
					if (match >= end_match)
						return false;
				}
				const CharType* const end_class = match - 1;
				if (match >= end_match || *match != *(CharType*) obj->getCanonicalChar(CHAR_GDML_MATCH_ANY))
				{
					if (!className(obj, flags, char_class, end_class, *search++))
						return false;
				}
				else {
					++match;
					for (;;)
					{
						if (aux(obj, flags, search, end_search, match, end_match))
							return true;
							
						if (search < end_search)
						{
							if (!className(obj, flags, char_class, end_class, *search++))
								return false;
						}
						else
							return false;
					}
				}
			}
			else if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_FLAG_SET))
			{
				c = *match++;
				if (c == *(CharType*) obj->getCanonicalChar(TextType::CHAR_LOWER_S) ||
					c == *(CharType*) obj->getCanonicalChar(TextType::CHAR_UPPER_S))
				{
					flags &= ~SLEUTH_INSENSITIVE;
				}
			}
			else if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_FLAG_CLEAR))
			{
				c = *match++;
				if (c == *(CharType*) obj->getCanonicalChar(TextType::CHAR_LOWER_S) ||
					c == *(CharType*) obj->getCanonicalChar(TextType::CHAR_UPPER_S))
				{
					flags |= SLEUTH_INSENSITIVE;
				}
			}
		}

		if (search < end_search)
			return false;

		return true;
	}

	// See if a character is a member of a class.
	// Japanese version operates on short-based buffer,
	// instead of SCHAR-based.
	static bool className(Jrd::TextType* obj, USHORT flags,
		const CharType* char_class, const CharType* const end_class, 
		CharType character)
	{
		fb_assert(char_class != NULL);
		fb_assert(end_class != NULL);
		fb_assert(char_class <= end_class);
		fb_assert(obj->getCanonicalWidth() == sizeof(CharType));

		bool result = true;

		if (*char_class == *(CharType*) obj->getCanonicalChar(CHAR_GDML_NOT)) {
			++char_class;
			result = false;
		}

		while (char_class < end_class) {
			const CharType c = *char_class++;
			if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE)) {
				if (*char_class++ == character)
					return true;
			}
			else if (*char_class == *(CharType*) obj->getCanonicalChar(CHAR_GDML_RANGE)) {
				char_class += 2;
				if (character >= c && character <= char_class[-1])
					return result;
			}
			else if (character == c)
				return result;
		}

		return !result;
	}

	// Merge the matching pattern and control strings to give a cannonical
	// matching pattern.  Return the length of the combined string. 
	//
	// What this routine does is to take the language template, strip off 
	// the prefix and put it in the output string, then parse the definitions
	// into an array of character pointers.  The index array is the defined
	// character.   The routine then takes the actual match pattern and uses
	// the characters in it to index into the definitions to produce an equivalent
	// pattern in the cannonical language.
	//
	// The silly loop setting *v++ to zero initializes the array up to the
	// highest character defined (also max_op).  Believe it or not, that part
	// is not a bug.
	static ULONG actualMerge(MemoryPool& pool, Jrd::TextType* obj,
		const CharType* match, SLONG match_bytes,
		const CharType* control, SLONG control_bytes,
		CharType* combined, SLONG combined_bytes)
	{
		fb_assert(match != NULL);
		fb_assert(control != NULL);
		fb_assert(combined != NULL);

		fb_assert((match_bytes % sizeof(CharType)) == 0);
		fb_assert((control_bytes % sizeof(CharType)) == 0);
		fb_assert(obj->getCanonicalWidth() == sizeof(CharType));

		const CharType* const end_match = match + (match_bytes / sizeof(CharType));
		const CharType* const end_control = control + (control_bytes / sizeof(CharType));

		CharType max_op = 0;
		CharType* comb = combined;
		CharType* vector[256];
		CharType** v = vector;
		CharType temp[256];
		CharType* t = temp;

		// Parse control string into substitution strings and initializing string

		while (control < end_control) {
			CharType c = *control++;
			if (*control == *(CharType*) obj->getCanonicalChar(CHAR_GDML_SUBSTITUTE)) {
				/* Note: don't allow substitution characters larger than vector */
				CharType** const end_vector =
					vector + (((int) c < FB_NELEM(vector)) ? c : 0);
				while (v <= end_vector)
					*v++ = 0;
				*end_vector = t;
				++control;
				while (control < end_control) {
					c = *control++;
					if ((t > temp && t[-1] == *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE)) ||
						((c != *(CharType*) obj->getCanonicalChar(CHAR_GDML_COMMA)) &&
						(c != *(CharType*) obj->getCanonicalChar(CHAR_GDML_RPAREN))))
					{
						*t++ = c;
					}
					else
						break;
				}
				*t++ = 0;
			}
			else if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE) && control < end_control)
				*comb++ = *control++;
			else if (c == *(CharType*) obj->getCanonicalChar(CHAR_GDML_RPAREN))
				break;
			else if (c != *(CharType*) obj->getCanonicalChar(CHAR_GDML_LPAREN))
				*comb++ = c;
		}

		max_op = v - vector;

		// Interpret matching string, substituting where appropriate

		while (match < end_match) {
			const CharType c = *match++;

			// if we've got a defined character, slurp the definition

			CharType* p;
			if (c <= max_op && (p = vector[c])) {
				while (*p)
					*comb++ = *p++;

				// if we've got the definition of a quote character, 
				// slurp the next character too

				if (comb > combined && comb[-1] == *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE) && *match)
					*comb++ = *match++;
			}

			// at this point we've got a non-match, but as it might be one of ours, quote it

			else {
				if ((((size_t) c) < FB_NELEM(SLEUTH_SPECIAL)) && SLEUTH_SPECIAL[c] &&
					comb > combined && comb[-1] != *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE))
				{
					*comb++ = *(CharType*) obj->getCanonicalChar(CHAR_GDML_QUOTE);
				}
				*comb++ = c;
			}
		}

		// Put in trailing stuff

		while (control < end_control)
			*comb++ = *control++;

		// YYY - need to add code watching for overflow of combined

		return (comb - combined) * sizeof(CharType);
	}

private:
	static const int SLEUTH_INSENSITIVE;
};

template <typename StrConverter, typename CharType>
const int SleuthMatcher<StrConverter, CharType>::SLEUTH_INSENSITIVE	= 1;


template <typename pStartsMatcher, typename pContainsMatcher, typename pLikeMatcher,
	typename pSimilarToMatcher, typename pMatchesMatcher, typename pSleuthMatcher>
class CollationImpl : public Collation
{
public:
	CollationImpl(TTYPE_ID a_type, texttype* a_tt, CharSet* a_cs)
		: Collation(a_type, a_tt, a_cs)
	{
	}

	virtual bool matches(MemoryPool& pool, const UCHAR* a, SLONG b, const UCHAR* c, SLONG d)
	{
		return pMatchesMatcher::evaluate(pool, this, a, b, c, d);
	}

	virtual bool sleuthCheck(MemoryPool& pool, USHORT a, const UCHAR* b,
		SLONG c, const UCHAR* d, SLONG e)
	{
		return pSleuthMatcher::check(pool, this, a, b, c, d, e);
	}

	virtual ULONG sleuthMerge(MemoryPool& pool, const UCHAR* a, SLONG b,
		const UCHAR* c, SLONG d, UCHAR* e, SLONG f)
	{
		return pSleuthMatcher::merge(pool, this, a, b, c, d, e, f);
	}

	virtual bool starts(MemoryPool& pool, const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl)
	{
		return pStartsMatcher::evaluate(pool, this, s, sl, p, pl);
	}

	virtual PatternMatcher* createStartsMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl)
	{
		return pStartsMatcher::create(pool, this, p, pl);
	}

	virtual bool like(MemoryPool& pool, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length)
	{
		return pLikeMatcher::evaluate(pool, this, s, sl, p, pl, escape, escape_length,
			getCharSet()->getSqlMatchAny(), getCharSet()->getSqlMatchAnyLength(),
			getCharSet()->getSqlMatchOne(), getCharSet()->getSqlMatchOneLength());
	}

	virtual PatternMatcher* createLikeMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl,
		const UCHAR* escape, SLONG escape_length)
	{
		return pLikeMatcher::create(pool, this, p, pl, escape, escape_length,
			getCharSet()->getSqlMatchAny(), getCharSet()->getSqlMatchAnyLength(),
			getCharSet()->getSqlMatchOne(), getCharSet()->getSqlMatchOneLength());
	}

	virtual bool similarTo(MemoryPool& pool, const UCHAR* s, SLONG sl,
		const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length)
	{
		return pSimilarToMatcher::evaluate(pool, this, s, sl, p, pl, escape, escape_length);
	}

	virtual PatternMatcher* createSimilarToMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl,
		const UCHAR* escape, SLONG escape_length)
	{
		return pSimilarToMatcher::create(pool, this, p, pl, escape, escape_length);
	}

	virtual bool contains(MemoryPool& pool, const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl)
	{
		return pContainsMatcher::evaluate(pool, this, s, sl, p, pl);
	}

	virtual PatternMatcher* createContainsMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl)
	{
		return pContainsMatcher::create(pool, this, p, pl);
	}
};

using namespace Firebird;

typedef StartsMatcher<NullStrConverter, UCHAR> StartsMatcherUCharDirect;
typedef StartsMatcher<CanonicalConverter<NullStrConverter>, UCHAR> StartsMatcherUCharCanonical;

typedef ContainsMatcher<UpcaseConverter<NullStrConverter>, UCHAR> ContainsMatcherUCharDirect;
//typedef ContainsMatcher<UpcaseConverter<NullStrConverter>, USHORT> ContainsMatcherUShortDirect;
//typedef ContainsMatcher<UpcaseConverter<NullStrConverter>, ULONG> ContainsMatcherULongDirect;

typedef MatchesMatcher<CanonicalConverter<NullStrConverter>, UCHAR> MatchesMatcherUCharCanonical;
typedef SleuthMatcher<CanonicalConverter<NullStrConverter>, UCHAR> SleuthMatcherUCharCanonical;
typedef LikeMatcher<CanonicalConverter<NullStrConverter>, UCHAR> LikeMatcherUCharCanonical;
typedef SimilarToMatcher<CanonicalConverter<NullStrConverter>, UCHAR> SimilarToMatcherUCharCanonical;
typedef ContainsMatcher<CanonicalConverter<UpcaseConverter<NullStrConverter> >, UCHAR> ContainsMatcherUCharCanonical;

typedef MatchesMatcher<CanonicalConverter<NullStrConverter>, USHORT> MatchesMatcherUShortCanonical;
typedef SleuthMatcher<CanonicalConverter<NullStrConverter>, USHORT> SleuthMatcherUShortCanonical;
typedef LikeMatcher<CanonicalConverter<NullStrConverter>, USHORT> LikeMatcherUShortCanonical;
typedef SimilarToMatcher<CanonicalConverter<NullStrConverter>, USHORT> SimilarToMatcherUShortCanonical;
typedef ContainsMatcher<CanonicalConverter<UpcaseConverter<NullStrConverter> >, USHORT> ContainsMatcherUShortCanonical;

typedef MatchesMatcher<CanonicalConverter<NullStrConverter>, ULONG> MatchesMatcherULongCanonical;
typedef SleuthMatcher<CanonicalConverter<NullStrConverter>, ULONG> SleuthMatcherULongCanonical;
typedef LikeMatcher<CanonicalConverter<NullStrConverter>, ULONG> LikeMatcherULongCanonical;
typedef SimilarToMatcher<CanonicalConverter<NullStrConverter>, ULONG> SimilarToMatcherULongCanonical;
typedef ContainsMatcher<CanonicalConverter<UpcaseConverter<NullStrConverter> >, ULONG> ContainsMatcherULongCanonical;

}	// namespace


//-------------


namespace Jrd {


Collation* Collation::createInstance(MemoryPool& pool, TTYPE_ID id, texttype* tt, CharSet* cs)
{
	fb_assert(tt->texttype_canonical_width == 1 ||
			  tt->texttype_canonical_width == 2 ||
			  tt->texttype_canonical_width == 4);

	switch (tt->texttype_canonical_width)
	{
		case 1:
			if (tt->texttype_flags & TEXTTYPE_DIRECT_MATCH)
			{
				return FB_NEW(pool) CollationImpl<StartsMatcherUCharDirect, ContainsMatcherUCharDirect,
					LikeMatcherUCharCanonical, SimilarToMatcherUCharCanonical,
					MatchesMatcherUCharCanonical, SleuthMatcherUCharCanonical>(id, tt, cs);
			}

			return FB_NEW(pool) CollationImpl<StartsMatcherUCharCanonical, ContainsMatcherUCharCanonical,
				LikeMatcherUCharCanonical, SimilarToMatcherUCharCanonical,
				MatchesMatcherUCharCanonical, SleuthMatcherUCharCanonical>(id, tt, cs);

		case 2:
			if (tt->texttype_flags & TEXTTYPE_DIRECT_MATCH)
			{
				return FB_NEW(pool) CollationImpl<StartsMatcherUCharDirect, ContainsMatcherUCharDirect,
					LikeMatcherUShortCanonical, SimilarToMatcherUShortCanonical,
					MatchesMatcherUShortCanonical, SleuthMatcherUShortCanonical>(id, tt, cs);
			}

			return FB_NEW(pool) CollationImpl<StartsMatcherUCharCanonical, ContainsMatcherUShortCanonical,
				LikeMatcherUShortCanonical, SimilarToMatcherUShortCanonical,
				MatchesMatcherUShortCanonical, SleuthMatcherUShortCanonical>(id, tt, cs);

		case 4:
			if (tt->texttype_flags & TEXTTYPE_DIRECT_MATCH)
			{
				return FB_NEW(pool) CollationImpl<StartsMatcherUCharDirect, ContainsMatcherUCharDirect,
					LikeMatcherULongCanonical, SimilarToMatcherULongCanonical,
					MatchesMatcherULongCanonical, SleuthMatcherULongCanonical>(id, tt, cs);
			}

			return FB_NEW(pool) CollationImpl<StartsMatcherUCharCanonical, ContainsMatcherULongCanonical,
				LikeMatcherULongCanonical, SimilarToMatcherULongCanonical,
				MatchesMatcherULongCanonical, SleuthMatcherULongCanonical>(id, tt, cs);
	}

	fb_assert(false);	
	return NULL;	// compiler silencer
}


void Collation::destroy()
{
	if (tt->texttype_fn_destroy)
		tt->texttype_fn_destroy(tt);

	delete tt;

	if (existenceLock)
	{
		// Establish a thread context.
		ThreadContextHolder tdbb;

		tdbb->setDatabase(existenceLock->lck_dbb);
		tdbb->setAttachment(existenceLock->lck_attachment);
		Jrd::ContextPoolHolder context(tdbb, 0);

		LCK_release(tdbb, existenceLock);

		delete existenceLock;
		existenceLock = NULL;
	}
}


void Collation::incUseCount(thread_db* tdbb)
{
	++useCount;
}


void Collation::decUseCount(thread_db* tdbb)
{
	if (--useCount == 0)
	{
		if (existenceLock)
			LCK_re_post(tdbb, existenceLock);
	}
}


}	// namespace Jrd

