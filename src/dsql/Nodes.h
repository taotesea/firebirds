/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef DSQL_NODES_H
#define DSQL_NODES_H

#include "../jrd/jrd.h"
#include "../dsql/DsqlCompilerScratch.h"
#include "../dsql/Visitors.h"
#include "../common/classes/array.h"
#include "../common/classes/NestConst.h"
#include <functional>
#include <type_traits>

namespace Jrd {

class AggregateSort;
class CompilerScratch;
class SubQuery;
class Cursor;
class Node;
class NodePrinter;
class ExprNode;
class NodeRefsHolder;
class OptimizerBlk;
class OptimizerRetrieval;
class RecordSource;
class RseNode;
class SlidingWindow;
class TypeClause;
class ValueExprNode;


// Must be less then MAX_SSHORT. Not used for static arrays.
const int MAX_CONJUNCTS	= 32000;

// New: MAX_STREAMS should be a multiple of BITS_PER_LONG (32 and hard to believe it will change)

const StreamType INVALID_STREAM = ~StreamType(0);
const StreamType MAX_STREAMS = 4096;

const StreamType STREAM_MAP_LENGTH = MAX_STREAMS + 2;

// New formula is simply MAX_STREAMS / BITS_PER_LONG
const int OPT_STREAM_BITS = MAX_STREAMS / BITS_PER_LONG; // 128 with 4096 streams

typedef Firebird::HalfStaticArray<StreamType, OPT_STATIC_ITEMS> StreamList;
typedef Firebird::SortedArray<StreamType> SortedStreamList;

typedef Firebird::Array<NestConst<ValueExprNode> > NestValueArray;


class Printable
{
public:
	virtual ~Printable()
	{
	}

public:
	void print(NodePrinter& printer) const;

	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;
};


template <typename T>
class RegisterNode
{
public:
	explicit RegisterNode(UCHAR blr)
	{
		PAR_register(blr, T::parse);
	}
};

template <typename T>
class RegisterBoolNode
{
public:
	explicit RegisterBoolNode(UCHAR blr)
	{
		PAR_register(blr, T::parse);
	}
};


class Node : public Printable
{
public:
	explicit Node(MemoryPool& pool)
		: line(0),
		  column(0)
	{
	}

	virtual ~Node()
	{
	}

	// Compile a parsed statement into something more interesting.
	template <typename T>
	static T* doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T>& node)
	{
		if (!node)
			return NULL;

		return node->dsqlPass(dsqlScratch);
	}

	// Compile a parsed statement into something more interesting and assign it to target.
	template <typename T1, typename T2>
	static void doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T1>& target, NestConst<T2>& node)
	{
		if (!node)
			target = NULL;
		else
			target = node->dsqlPass(dsqlScratch);
	}

	// Changes dsqlScratch->isPsql() value, calls doDsqlPass and restore dsqlScratch->isPsql().
	template <typename T>
	static T* doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T>& node, bool psql)
	{
		PsqlChanger changer(dsqlScratch, psql);
		return doDsqlPass(dsqlScratch, node);
	}

	// Changes dsqlScratch->isPsql() value, calls doDsqlPass and restore dsqlScratch->isPsql().
	template <typename T1, typename T2>
	static void doDsqlPass(DsqlCompilerScratch* dsqlScratch, NestConst<T1>& target, NestConst<T2>& node, bool psql)
	{
		PsqlChanger changer(dsqlScratch, psql);
		doDsqlPass(dsqlScratch, target, node);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
	}

	virtual Node* dsqlPass(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		return this;
	}

public:
	ULONG line;
	ULONG column;
};


class DdlNode;

class DdlNode : public Node
{
public:
	explicit DdlNode(MemoryPool& pool)
		: Node(pool)
	{
	}

	static bool deleteSecurityClass(thread_db* tdbb, jrd_tra* transaction,
		const Firebird::MetaName& secClass);

	static void storePrivileges(thread_db* tdbb, jrd_tra* transaction,
		const Firebird::MetaName& name, int type, const char* privileges);

public:
	// Check permission on DDL operation. Return true if everything is OK.
	// Raise an exception for bad permission.
	// If returns false permissions will be check in old style at vio level as well as while direct RDB$ tables modify.
	virtual bool checkPermission(thread_db* tdbb, jrd_tra* transaction) = 0;

	// Set the scratch's transaction when executing a node. Fact of accessing the scratch during
	// execution is a hack.
	void executeDdl(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction)
	{
		// dsqlScratch should be NULL with CREATE DATABASE.
		if (dsqlScratch)
			dsqlScratch->setTransaction(transaction);

		checkPermission(tdbb, transaction);
		execute(tdbb, dsqlScratch, transaction);
	}

	virtual DdlNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		dsqlScratch->getStatement()->setType(DsqlCompiledStatement::TYPE_DDL);
		return this;
	}

public:
	enum DdlTriggerWhen { DTW_BEFORE, DTW_AFTER };

	static void executeDdlTrigger(thread_db* tdbb, jrd_tra* transaction,
		DdlTriggerWhen when, int action, const Firebird::MetaName& objectName,
		const Firebird::MetaName& oldNewObjectName, const Firebird::string& sqlText);

protected:
	typedef Firebird::Pair<Firebird::Left<Firebird::MetaName, bid> > MetaNameBidPair;
	typedef Firebird::GenericMap<MetaNameBidPair> MetaNameBidMap;

	// Return exception code based on combination of create and alter clauses.
	static ISC_STATUS createAlterCode(bool create, bool alter, ISC_STATUS createCode,
		ISC_STATUS alterCode, ISC_STATUS createOrAlterCode)
	{
		if (create && alter)
			return createOrAlterCode;

		if (create)
			return createCode;

		if (alter)
			return alterCode;

		fb_assert(false);
		return 0;
	}

	void executeDdlTrigger(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction,
		DdlTriggerWhen when, int action, const Firebird::MetaName& objectName,
		const Firebird::MetaName& oldNewObjectName);
	void storeGlobalField(thread_db* tdbb, jrd_tra* transaction, Firebird::MetaName& name,
		const TypeClause* field,
		const Firebird::string& computedSource = "",
		const BlrDebugWriter::BlrData& computedValue = BlrDebugWriter::BlrData());

public:
	// Prefix DDL exceptions. To be implemented in each command.
	// Attention: do not store temp strings in Arg::StatusVector,
	// when needed keep them permanently in command's node.
	virtual void putErrorPrefix(Firebird::Arg::StatusVector& statusVector) = 0;

	virtual void execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch, jrd_tra* transaction) = 0;

	virtual bool mustBeReplicated() const
	{
		return true;
	}
};


class TransactionNode : public Node
{
public:
	explicit TransactionNode(MemoryPool& pool)
		: Node(pool)
	{
	}

public:
	virtual TransactionNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		Node::dsqlPass(dsqlScratch);
		return this;
	}

	virtual void execute(thread_db* tdbb, dsql_req* request, jrd_tra** transaction) const = 0;
};


class SessionManagementNode : public Node
{
public:
	explicit SessionManagementNode(MemoryPool& pool)
		: Node(pool)
	{
	}

public:
	virtual SessionManagementNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		Node::dsqlPass(dsqlScratch);

		dsqlScratch->getStatement()->setType(DsqlCompiledStatement::TYPE_SESSION_MANAGEMENT);

		return this;
	}

	virtual void execute(thread_db* tdbb, dsql_req* request, jrd_tra** traHandle) const = 0;
};


class DmlNode : public Node
{
public:
	// DML node kinds
	enum Kind
	{
		KIND_STATEMENT,
		KIND_VALUE,
		KIND_BOOLEAN,
		KIND_REC_SOURCE,
		KIND_LIST
	};

	explicit DmlNode(MemoryPool& pool)
		: Node(pool)
	{
	}

	// Merge missing values, computed values, validation expressions, and views into a parsed request.
	template <typename T> static void doPass1(thread_db* tdbb, CompilerScratch* csb, T** node)
	{
		if (!*node)
			return;

		*node = (*node)->pass1(tdbb, csb);
	}

public:
	virtual Kind getKind() = 0;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch) = 0;
	virtual DmlNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual DmlNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual DmlNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
};


template <typename T, typename T::Type typeConst>
class TypedNode : public T
{
public:
	explicit TypedNode(MemoryPool& pool)
		: T(typeConst, pool)
	{
	}

public:
	const static typename T::Type TYPE = typeConst;
};


template <typename To, typename From> static To* nodeAs(From* fromNode)
{
	return fromNode && fromNode->type == To::TYPE ? static_cast<To*>(fromNode) : NULL;
}

template <typename To, typename From> static To* nodeAs(NestConst<From>& fromNode)
{
	return fromNode && fromNode->type == To::TYPE ? static_cast<To*>(fromNode.getObject()) : NULL;
}

template <typename To, typename From> static const To* nodeAs(const From* fromNode)
{
	return fromNode && fromNode->type == To::TYPE ? static_cast<const To*>(fromNode) : NULL;
}

template <typename To, typename From> static const To* nodeAs(const NestConst<From>& fromNode)
{
	return fromNode && fromNode->type == To::TYPE ? static_cast<const To*>(fromNode.getObject()) : NULL;
}

template <typename To, typename From> static bool nodeIs(const From* fromNode)
{
	return fromNode && fromNode->type == To::TYPE;
}

template <typename To, typename From> static bool nodeIs(const NestConst<From>& fromNode)
{
	return fromNode && fromNode->type == To::TYPE;
}


class NodeRefsHolder : public Firebird::PermanentStorage
{
public:
	NodeRefsHolder(MemoryPool& pool)
		: PermanentStorage(pool),
		  refs(pool)
	{
	}

	template <typename T> void add(const NestConst<T>& node)
	{
		static_assert(std::is_base_of<ExprNode, T>::value, "T must be derived from ExprNode");

		static_assert(
			std::is_convertible<
				decltype(const_cast<T*>(node.getObject())->pass1(
					(thread_db*) nullptr, (CompilerScratch*) nullptr)),
				decltype(const_cast<T*>(node.getObject()))
			>::value,
			"pass1 problem");

		static_assert(
			std::is_convertible<
				decltype(const_cast<T*>(node.getObject())->pass2(
					(thread_db*) nullptr, (CompilerScratch*) nullptr)),
				decltype(const_cast<T*>(node.getObject()))
			>::value,
			"pass2 problem");

		static_assert(
			std::is_convertible<
				decltype(const_cast<T*>(node.getObject())->dsqlFieldRemapper(*(FieldRemapper*) nullptr)),
				decltype(const_cast<T*>(node.getObject()))
			>::value,
			"dsqlFieldRemapper problem");

		T** ptr = const_cast<T**> (node.getAddress());
		fb_assert(ptr);

		refs.add(reinterpret_cast<ExprNode**>(ptr));
	}

public:
	Firebird::HalfStaticArray<ExprNode**, 8> refs;
};


class ExprNode : public DmlNode
{
public:
	enum Type
	{
		// Value types
		TYPE_AGGREGATE,
		TYPE_ALIAS,
		TYPE_ARITHMETIC,
		TYPE_ARRAY,
		TYPE_AT,
		TYPE_BOOL_AS_VALUE,
		TYPE_CAST,
		TYPE_COALESCE,
		TYPE_COLLATE,
		TYPE_CONCATENATE,
		TYPE_CURRENT_DATE,
		TYPE_CURRENT_TIME,
		TYPE_CURRENT_TIMESTAMP,
		TYPE_CURRENT_ROLE,
		TYPE_CURRENT_USER,
		TYPE_DERIVED_EXPR,
		TYPE_DECODE,
		TYPE_DEFAULT,
		TYPE_DERIVED_FIELD,
		TYPE_DOMAIN_VALIDATION,
		TYPE_EXTRACT,
		TYPE_FIELD,
		TYPE_GEN_ID,
		TYPE_INTERNAL_INFO,
		TYPE_LITERAL,
		TYPE_LOCAL_TIME,
		TYPE_LOCAL_TIMESTAMP,
		TYPE_MAP,
		TYPE_NEGATE,
		TYPE_NULL,
		TYPE_ORDER,
		TYPE_OVER,
		TYPE_PARAMETER,
		TYPE_RECORD_KEY,
		TYPE_SCALAR,
		TYPE_STMT_EXPR,
		TYPE_STR_CASE,
		TYPE_STR_LEN,
		TYPE_SUBQUERY,
		TYPE_SUBSTRING,
		TYPE_SUBSTRING_SIMILAR,
		TYPE_SYSFUNC_CALL,
		TYPE_TRIM,
		TYPE_UDF_CALL,
		TYPE_VALUE_IF,
		TYPE_VARIABLE,
		TYPE_WINDOW_CLAUSE,
		TYPE_WINDOW_CLAUSE_FRAME,
		TYPE_WINDOW_CLAUSE_FRAME_EXTENT,

		// Bool types
		TYPE_BINARY_BOOL,
		TYPE_COMPARATIVE_BOOL,
		TYPE_MISSING_BOOL,
		TYPE_NOT_BOOL,
		TYPE_RSE_BOOL,

		// RecordSource types
		TYPE_AGGREGATE_SOURCE,
		TYPE_PROCEDURE,
		TYPE_RELATION,
		TYPE_RSE,
		TYPE_SELECT_EXPR,
		TYPE_UNION,
		TYPE_WINDOW,

		// List types
		TYPE_REC_SOURCE_LIST,
		TYPE_VALUE_LIST
	};

	// Generic flags.
	static const unsigned FLAG_INVARIANT	= 0x01;	// Node is recognized as being invariant.

	// Boolean flags.
	static const unsigned FLAG_DEOPTIMIZE	= 0x02;	// Boolean which requires deoptimization.
	static const unsigned FLAG_RESIDUAL		= 0x04;	// Boolean which must remain residual.
	static const unsigned FLAG_ANSI_NOT		= 0x08;	// ANY/ALL predicate is prefixed with a NOT one.

	// Value flags.
	static const unsigned FLAG_DOUBLE		= 0x10;
	static const unsigned FLAG_DATE			= 0x20;
	static const unsigned FLAG_DECFLOAT		= 0x40;
	static const unsigned FLAG_VALUE		= 0x80;	// Full value area required in impure space.
	static const unsigned FLAG_INT128		= 0x100;

	explicit ExprNode(Type aType, MemoryPool& pool)
		: DmlNode(pool),
		  type(aType),
		  nodFlags(0),
		  impureOffset(0)
	{
	}

	virtual const char* getCompatDialectVerb()
	{
		return NULL;
	}

	// Allocate and assign impure space for various nodes.
	template <typename T> static void doPass2(thread_db* tdbb, CompilerScratch* csb, T** node)
	{
		if (!*node)
			return;

		*node = (*node)->pass2(tdbb, csb);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor)
	{
		bool ret = false;

		NodeRefsHolder holder(visitor.getPool());
		getChildren(holder, true);

		for (auto i : holder.refs)
			ret |= visitor.visit(*i);

		return ret;
	}

	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor)
	{
		bool ret = false;

		NodeRefsHolder holder(visitor.getPool());
		getChildren(holder, true);

		for (auto i : holder.refs)
			ret |= visitor.visit(*i);

		return ret;
	}

	virtual bool dsqlFieldFinder(FieldFinder& visitor)
	{
		bool ret = false;

		NodeRefsHolder holder(visitor.getPool());
		getChildren(holder, true);

		for (auto i : holder.refs)
			ret |= visitor.visit(*i);

		return ret;
	}

	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor)
	{
		bool ret = false;

		NodeRefsHolder holder(visitor.dsqlScratch->getPool());
		getChildren(holder, true);

		for (auto i : holder.refs)
			ret |= visitor.visit(*i);

		return ret;
	}

	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor)
	{
		bool ret = false;

		NodeRefsHolder holder(visitor.getPool());
		getChildren(holder, true);

		for (auto i : holder.refs)
			ret |= visitor.visit(*i);

		return ret;
	}

	virtual ExprNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		NodeRefsHolder holder(visitor.getPool());
		getChildren(holder, true);

		for (auto i : holder.refs)
		{
			if (*i)
				*i = (*i)->dsqlFieldRemapper(visitor);
		}

		return this;
	}

	template <typename T>
	static void doDsqlFieldRemapper(FieldRemapper& visitor, NestConst<T>& node)
	{
		if (node)
			node = node->dsqlFieldRemapper(visitor);
	}

	template <typename T1, typename T2>
	static void doDsqlFieldRemapper(FieldRemapper& visitor, NestConst<T1>& target, NestConst<T2> node)
	{
		target = node ? node->dsqlFieldRemapper(visitor) : NULL;
	}

	// Check if expression could return NULL or expression can turn NULL into a true/false.
	virtual bool possiblyUnknown(OptimizerBlk* opt);

	// Verify if this node is allowed in an unmapped boolean.
	virtual bool unmappable(CompilerScratch* csb, const MapNode* mapNode, StreamType shellStream);

	// Return all streams referenced by the expression.
	virtual void collectStreams(CompilerScratch* csb, SortedStreamList& streamList) const;

	virtual bool findStream(CompilerScratch* csb, StreamType stream)
	{
		SortedStreamList streams;
		collectStreams(csb, streams);

		return streams.exist(stream);
	}

	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;

	virtual ExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		DmlNode::dsqlPass(dsqlScratch);
		return this;
	}

	// Determine if two expression trees are the same.
	virtual bool sameAs(CompilerScratch* csb, const ExprNode* other, bool ignoreStreams) const;

	// See if node is presently computable.
	// A node is said to be computable, if all the streams involved
	// in that node are csb_active. The csb_active flag defines
	// all the streams available in the current scope of the query.
	virtual bool computable(CompilerScratch* csb, StreamType stream,
		bool allowOnlyCurrentStream, ValueExprNode* value = NULL);

	virtual void findDependentFromStreams(const OptimizerRetrieval* optRet,
		SortedStreamList* streamList);
	virtual ExprNode* pass1(thread_db* tdbb, CompilerScratch* csb);
	virtual ExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);
	virtual ExprNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;

public:
	const Type type;
	unsigned nodFlags;
	ULONG impureOffset;
};


class BoolExprNode : public ExprNode
{
public:
	BoolExprNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool)
	{
	}

	virtual Kind getKind()
	{
		return KIND_BOOLEAN;
	}

	virtual BoolExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ExprNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual BoolExprNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual BoolExprNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual BoolExprNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual void pass2Boolean1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
	}

	virtual void pass2Boolean2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
	}

	virtual BoolExprNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
	virtual bool execute(thread_db* tdbb, jrd_req* request) const = 0;
};

class ValueExprNode : public ExprNode
{
public:
	ValueExprNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool),
		  nodScale(0)
	{
		nodDesc.clear();
	}

public:
	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;

	virtual Kind getKind()
	{
		return KIND_VALUE;
	}

	virtual ValueExprNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ExprNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual bool setParameterType(DsqlCompilerScratch* /*dsqlScratch*/,
		std::function<void (dsc*)> /*makeDesc*/, bool /*forceVarChar*/)
	{
		return false;
	}

	virtual void setParameterName(dsql_par* parameter) const = 0;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc) = 0;

	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual ValueExprNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual ValueExprNode* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass2(tdbb, csb);
		return this;
	}

	// Compute descriptor for value expression.
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc) = 0;

	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const = 0;

public:
	SCHAR nodScale;
	dsc nodDesc;
};

template <typename T, typename ValueExprNode::Type typeConst>
class DsqlNode : public TypedNode<ValueExprNode, typeConst>
{
public:
	DsqlNode(MemoryPool& pool)
		: TypedNode<ValueExprNode, typeConst>(pool)
	{
	}

public:
	virtual void setParameterName(dsql_par* /*parameter*/) const
	{
		fb_assert(false);
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual void make(DsqlCompilerScratch* /*dsqlScratch*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual void getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* /*desc*/)
	{
		fb_assert(false);
	}

	virtual ValueExprNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return NULL;
	}

	virtual ValueExprNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return NULL;
	}

	virtual ValueExprNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	virtual dsc* execute(thread_db* /*tdbb*/, jrd_req* /*request*/) const
	{
		fb_assert(false);
		return NULL;
	}
};

class AggNode : public TypedNode<ValueExprNode, ExprNode::TYPE_AGGREGATE>
{
public:
	// Capabilities
	// works in a window frame
	static const unsigned CAP_SUPPORTS_WINDOW_FRAME	= 0x01;
	// respects window frame boundaries
	static const unsigned CAP_RESPECTS_WINDOW_FRAME	= 0x02 | CAP_SUPPORTS_WINDOW_FRAME;
	// wants aggPass/aggExecute calls in a window
	static const unsigned CAP_WANTS_AGG_CALLS		= 0x04;
	// wants winPass call in a window
	static const unsigned CAP_WANTS_WIN_PASS_CALL	= 0x08;

protected:
	struct AggInfo
	{
		AggInfo(const char* aName, UCHAR aBlr, UCHAR aDistinctBlr)
			: name(aName),
			  blr(aBlr),
			  distinctBlr(aDistinctBlr)
		{
		}

		const char* const name;
		const UCHAR blr;
		const UCHAR distinctBlr;
	};

	// Base factory to create instance of subclasses.
	class Factory : public AggInfo
	{
	public:
		explicit Factory(const char* aName)
			: AggInfo(aName, 0, 0)
		{
			next = factories;
			factories = this;
		}

		virtual AggNode* newInstance(MemoryPool& pool) const = 0;

	public:
		const Factory* next;
	};

public:
	// Concrete implementations for the factory.

	template <typename T>
	class RegisterFactory0 : public Factory
	{
	public:
		explicit RegisterFactory0(const char* aName)
			: Factory(aName)
		{
		}

		AggNode* newInstance(MemoryPool& pool) const
		{
			return FB_NEW T(pool);
		}
	};

	template <typename T, typename Type>
	class RegisterFactory1 : public Factory
	{
	public:
		explicit RegisterFactory1(const char* aName, Type aType)
			: Factory(aName),
			  type(aType)
		{
		}

		AggNode* newInstance(MemoryPool& pool) const
		{
			return FB_NEW T(pool, type);
		}

	public:
		const Type type;
	};

	template <typename T>
	class Register : public AggInfo
	{
	public:
		explicit Register(const char* aName, UCHAR blr, UCHAR blrDistinct)
			: AggInfo(aName, blr, blrDistinct),
			  registerNode1(blr),
			  registerNode2(blrDistinct)
		{
		}

		explicit Register(const char* aName, UCHAR blr)
			: AggInfo(aName, blr, blr),
			  registerNode1(blr),
			  registerNode2(blr)
		{
		}

	private:
		RegisterNode<T> registerNode1, registerNode2;
	};

public:
	explicit AggNode(MemoryPool& pool, const AggInfo& aAggInfo, bool aDistinct, bool aDialect1,
		ValueExprNode* aArg = NULL);

	static DmlNode* parse(thread_db* tdbb, MemoryPool& pool, CompilerScratch* csb, const UCHAR blrOp);

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ValueExprNode::getChildren(holder, dsql);
		holder.add(arg);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;

	virtual bool dsqlAggregateFinder(AggregateFinder& visitor);
	virtual bool dsqlAggregate2Finder(Aggregate2Finder& visitor);
	virtual bool dsqlInvalidReferenceFinder(InvalidReferenceFinder& visitor);
	virtual bool dsqlSubSelectFinder(SubSelectFinder& visitor);
	virtual ValueExprNode* dsqlFieldRemapper(FieldRemapper& visitor);

	virtual bool dsqlMatch(DsqlCompilerScratch* dsqlScratch, const ExprNode* other, bool ignoreMapCast) const;
	virtual void setParameterName(dsql_par* parameter) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

	virtual AggNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ValueExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual AggNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual bool possiblyUnknown(OptimizerBlk* /*opt*/)
	{
		return true;
	}

	virtual void collectStreams(CompilerScratch* /*csb*/, SortedStreamList& /*streamList*/) const
	{
		// ASF: Although in v2.5 the visitor happens normally for the node childs, nod_count has
		// been set to 0 in CMP_pass2, so that doesn't happens.
		return;
	}

	virtual bool unmappable(CompilerScratch* /*csb*/, const MapNode* /*mapNode*/, StreamType /*shellStream*/)
	{
		return false;
	}

	virtual dsc* winPass(thread_db* /*tdbb*/, jrd_req* /*request*/, SlidingWindow* /*window*/) const
	{
		return NULL;
	}

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const = 0;	// pure, but defined
	virtual void aggFinish(thread_db* tdbb, jrd_req* request) const;
	virtual bool aggPass(thread_db* tdbb, jrd_req* request) const;
	virtual dsc* execute(thread_db* tdbb, jrd_req* request) const;

	virtual unsigned getCapabilities() const = 0;
	virtual void aggPass(thread_db* tdbb, jrd_req* request, dsc* desc) const = 0;
	virtual dsc* aggExecute(thread_db* tdbb, jrd_req* request) const = 0;

	virtual AggNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

protected:
	virtual void parseArgs(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, unsigned count)
	{
		fb_assert(count == 0);
	}

	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/ = 0;

public:
	const AggInfo& aggInfo;
	NestConst<ValueExprNode> arg;
	const AggregateSort* asb;
	bool distinct;
	bool dialect1;
	bool indexed;

private:
	static Factory* factories;
};


// Base class for window functions.
class WinFuncNode : public AggNode
{
public:
	explicit WinFuncNode(MemoryPool& pool, const AggInfo& aAggInfo, ValueExprNode* aArg = NULL);

public:
	virtual void aggPass(thread_db* tdbb, jrd_req* request, dsc* desc) const
	{
	}

	virtual dsc* aggExecute(thread_db* tdbb, jrd_req* request) const
	{
		return NULL;
	}
};


class RecordSourceNode : public ExprNode
{
public:
	static const unsigned DFLAG_SINGLETON				= 0x01;
	static const unsigned DFLAG_VALUE					= 0x02;
	static const unsigned DFLAG_RECURSIVE				= 0x04;	// recursive member of recursive CTE
	static const unsigned DFLAG_DERIVED					= 0x08;
	static const unsigned DFLAG_DT_IGNORE_COLUMN_CHECK	= 0x10;
	static const unsigned DFLAG_DT_CTE_USED				= 0x20;
	static const unsigned DFLAG_CURSOR					= 0x40;

	RecordSourceNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool),
		  dsqlFlags(0),
		  dsqlContext(NULL),
		  stream(INVALID_STREAM)
	{
	}

	virtual Kind getKind()
	{
		return KIND_REC_SOURCE;
	}

	virtual StreamType getStream() const
	{
		return stream;
	}

	void setStream(StreamType value)
	{
		stream = value;
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;

	virtual RecordSourceNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ExprNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual RecordSourceNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual RecordSourceNode* copy(thread_db* tdbb, NodeCopier& copier) const = 0;
	virtual void ignoreDbKey(thread_db* tdbb, CompilerScratch* csb) const = 0;
	virtual RecordSourceNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual void pass1Source(thread_db* tdbb, CompilerScratch* csb, RseNode* rse,
		BoolExprNode** boolean, RecordSourceNodeStack& stack) = 0;
	virtual RecordSourceNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual void pass2Rse(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual bool containsStream(StreamType checkStream) const = 0;
	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}

	virtual bool possiblyUnknown(OptimizerBlk* /*opt*/)
	{
		return true;
	}

	virtual bool unmappable(CompilerScratch* /*csb*/, const MapNode* /*mapNode*/, StreamType /*shellStream*/)
	{
		return false;
	}

	virtual void collectStreams(CompilerScratch* /*csb*/, SortedStreamList& streamList) const
	{
		if (!streamList.exist(getStream()))
			streamList.add(getStream());
	}

	virtual bool sameAs(CompilerScratch* /*csb*/, const ExprNode* /*other*/, bool /*ignoreStreams*/) const
	{
		return false;
	}

	// Identify all of the streams for which a dbkey may need to be carried through a sort.
	virtual void computeDbKeyStreams(StreamList& streamList) const = 0;

	// Identify the streams that make up an RseNode.
	virtual void computeRseStreams(StreamList& streamList) const
	{
		streamList.add(getStream());
	}

	virtual RecordSource* compile(thread_db* tdbb, OptimizerBlk* opt, bool innerSubStream) = 0;

public:
	unsigned dsqlFlags;
	dsql_ctx* dsqlContext;

protected:
	StreamType stream;
};


class ListExprNode : public ExprNode
{
public:
	ListExprNode(Type aType, MemoryPool& pool)
		: ExprNode(aType, pool)
	{
	}

	virtual Kind getKind()
	{
		return KIND_LIST;
	}

	virtual void genBlr(DsqlCompilerScratch* /*dsqlScratch*/)
	{
		fb_assert(false);
	}
};

// Container for a list of value expressions.
class ValueListNode : public TypedNode<ListExprNode, ExprNode::TYPE_VALUE_LIST>
{
public:
	ValueListNode(MemoryPool& pool, unsigned count)
		: TypedNode<ListExprNode, ExprNode::TYPE_VALUE_LIST>(pool),
		  items(pool, INITIAL_CAPACITY)
	{
		items.resize(count);

		for (unsigned i = 0; i < count; ++i)
			items[i] = NULL;
	}

	ValueListNode(MemoryPool& pool, ValueExprNode* arg1)
		: TypedNode<ListExprNode, ExprNode::TYPE_VALUE_LIST>(pool),
		  items(pool, INITIAL_CAPACITY)
	{
		items.push(arg1);
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ListExprNode::getChildren(holder, dsql);

		for (auto& item : items)
			holder.add(item);
	}

	ValueListNode* add(ValueExprNode* argn)
	{
		items.add(argn);
		return this;
	}

	ValueListNode* addFront(ValueExprNode* argn)
	{
		items.insert(0, argn);
		return this;
	}

	void clear()
	{
		items.clear();
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;

	virtual ValueListNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		ValueListNode* node = FB_NEW_POOL(dsqlScratch->getPool()) ValueListNode(dsqlScratch->getPool(),
			items.getCount());

		NestConst<ValueExprNode>* dst = node->items.begin();

		for (NestConst<ValueExprNode>* src = items.begin(); src != items.end(); ++src, ++dst)
			*dst = doDsqlPass(dsqlScratch, *src);

		return node;
	}

	virtual ValueListNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual ValueListNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual ValueListNode* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass2(tdbb, csb);
		return this;
	}

	virtual ValueListNode* copy(thread_db* tdbb, NodeCopier& copier) const
	{
		ValueListNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) ValueListNode(*tdbb->getDefaultPool(),
			items.getCount());

		NestConst<ValueExprNode>* j = node->items.begin();

		for (const NestConst<ValueExprNode>* i = items.begin(); i != items.end(); ++i, ++j)
			*j = copier.copy(tdbb, *i);

		return node;
	}

public:
	NestValueArray items;

private:
	static const unsigned INITIAL_CAPACITY = 4;
};

// Container for a list of record source expressions.
class RecSourceListNode : public TypedNode<ListExprNode, ExprNode::TYPE_REC_SOURCE_LIST>
{
public:
	RecSourceListNode(MemoryPool& pool, unsigned count);
	RecSourceListNode(MemoryPool& pool, RecordSourceNode* arg1);

	RecSourceListNode* add(RecordSourceNode* argn)
	{
		items.add(argn);
		return this;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		ListExprNode::getChildren(holder, dsql);

		for (auto& item : items)
			holder.add(item);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;

	virtual RecSourceListNode* dsqlPass(DsqlCompilerScratch* dsqlScratch);

	virtual RecSourceListNode* dsqlFieldRemapper(FieldRemapper& visitor)
	{
		ExprNode::dsqlFieldRemapper(visitor);
		return this;
	}

	virtual RecSourceListNode* pass1(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass1(tdbb, csb);
		return this;
	}

	virtual RecSourceListNode* pass2(thread_db* tdbb, CompilerScratch* csb)
	{
		ExprNode::pass2(tdbb, csb);
		return this;
	}

	virtual RecSourceListNode* copy(thread_db* tdbb, NodeCopier& copier) const
	{
		fb_assert(false);
		return NULL;
	}

public:
	Firebird::Array<NestConst<RecordSourceNode> > items;
};


class StmtNode : public DmlNode
{
public:
	enum Type
	{
		TYPE_ASSIGNMENT,
		TYPE_BLOCK,
		TYPE_COMPOUND_STMT,
		TYPE_CONTINUE_LEAVE,
		TYPE_CURSOR_STMT,
		TYPE_DECLARE_CURSOR,
		TYPE_DECLARE_SUBFUNC,
		TYPE_DECLARE_SUBPROC,
		TYPE_DECLARE_VARIABLE,
		TYPE_ERASE,
		TYPE_ERROR_HANDLER,
		TYPE_EXCEPTION,
		TYPE_EXEC_BLOCK,
		TYPE_EXEC_PROCEDURE,
		TYPE_EXEC_STATEMENT,
		TYPE_EXIT,
		TYPE_IF,
		TYPE_IN_AUTO_TRANS,
		TYPE_INIT_VARIABLE,
		TYPE_FOR,
		TYPE_HANDLER,
		TYPE_LABEL,
		TYPE_LINE_COLUMN,
		TYPE_LOOP,
		TYPE_MERGE,
		TYPE_MERGE_SEND,
		TYPE_MESSAGE,
		TYPE_MODIFY,
		TYPE_POST_EVENT,
		TYPE_RECEIVE,
		TYPE_RETURN,
		TYPE_SAVEPOINT,
		TYPE_SAVEPOINT_ENCLOSE,
		TYPE_SELECT,
		TYPE_SESSION_MANAGEMENT_WRAPPER,
		TYPE_SET_GENERATOR,
		TYPE_STALL,
		TYPE_STORE,
		TYPE_SUSPEND,
		TYPE_UPDATE_OR_INSERT,
		TYPE_USER_SAVEPOINT,

		TYPE_EXT_INIT_PARAMETER,
		TYPE_EXT_TRIGGER
	};

	enum WhichTrigger
	{
		ALL_TRIGS = 0,
		PRE_TRIG = 1,
		POST_TRIG = 2
	};

	struct ExeState
	{
		ExeState(thread_db* tdbb, jrd_req* request, jrd_tra* transaction)
			: savedTdbb(tdbb),
			  oldPool(tdbb->getDefaultPool()),
			  oldRequest(tdbb->getRequest()),
			  oldTransaction(tdbb->getTransaction()),
			  errorPending(false),
			  catchDisabled(false),
			  whichEraseTrig(ALL_TRIGS),
			  whichStoTrig(ALL_TRIGS),
			  whichModTrig(ALL_TRIGS),
			  topNode(NULL),
			  prevNode(NULL),
			  exit(false)
		{
			savedTdbb->setTransaction(transaction);
			savedTdbb->setRequest(request);
		}

		~ExeState()
		{
			savedTdbb->setTransaction(oldTransaction);
			savedTdbb->setRequest(oldRequest);
		}

		thread_db* savedTdbb;
		MemoryPool* oldPool;		// Save the old pool to restore on exit.
		jrd_req* oldRequest;		// Save the old request to restore on exit.
		jrd_tra* oldTransaction;	// Save the old transcation to restore on exit.
		bool errorPending;			// Is there an error pending to be handled?
		bool catchDisabled;			// Catch errors so we can unwind cleanly.
		WhichTrigger whichEraseTrig;
		WhichTrigger whichStoTrig;
		WhichTrigger whichModTrig;
		const StmtNode* topNode;
		const StmtNode* prevNode;
		bool exit;					// Exit the looper when true.
	};

public:
	explicit StmtNode(Type aType, MemoryPool& pool)
		: DmlNode(pool),
		  type(aType),
		  parentStmt(NULL),
		  impureOffset(0),
		  hasLineColumn(false)
	{
	}

	template <typename T> T* as()
	{
		return type == T::TYPE ? static_cast<T*>(this) : NULL;
	}

	template <typename T> const T* as() const
	{
		return type == T::TYPE ? static_cast<const T*>(this) : NULL;
	}

	template <typename T> bool is() const
	{
		return type == T::TYPE;
	}

	template <typename T, typename T2> static T* as(T2* node)
	{
		return node ? node->template as<T>() : NULL;
	}

	template <typename T, typename T2> static const T* as(const T2* node)
	{
		return node ? node->template as<T>() : NULL;
	}

	template <typename T, typename T2> static bool is(const T2* node)
	{
		return node && node->template is<T>();
	}

	// Allocate and assign impure space for various nodes.
	template <typename T> static void doPass2(thread_db* tdbb, CompilerScratch* csb, T** node,
		StmtNode* parentStmt)
	{
		if (!*node)
			return;

		if (parentStmt)
			(*node)->parentStmt = parentStmt;

		*node = (*node)->pass2(tdbb, csb);
	}

	virtual Kind getKind()
	{
		return KIND_STATEMENT;
	}

	virtual StmtNode* dsqlPass(DsqlCompilerScratch* dsqlScratch)
	{
		DmlNode::dsqlPass(dsqlScratch);
		return this;
	}

	virtual StmtNode* pass1(thread_db* tdbb, CompilerScratch* csb) = 0;
	virtual StmtNode* pass2(thread_db* tdbb, CompilerScratch* csb) = 0;

	virtual StmtNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		Firebird::status_exception::raise(
			Firebird::Arg::Gds(isc_cannot_copy_stmt) <<
			Firebird::Arg::Num(int(type)));

		return NULL;
	}

	virtual const StmtNode* execute(thread_db* tdbb, jrd_req* request, ExeState* exeState) const = 0;

public:
	const Type type;
	NestConst<StmtNode> parentStmt;
	ULONG impureOffset;	// Inpure offset from request block.
	bool hasLineColumn;
};


// Used to represent nodes that don't have a specific BLR verb, i.e.,
// do not use RegisterNode.
class DsqlOnlyStmtNode : public StmtNode
{
public:
	explicit DsqlOnlyStmtNode(Type aType, MemoryPool& pool)
		: StmtNode(aType, pool)
	{
	}

public:
	virtual DsqlOnlyStmtNode* pass1(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return this;
	}

	virtual DsqlOnlyStmtNode* pass2(thread_db* /*tdbb*/, CompilerScratch* /*csb*/)
	{
		fb_assert(false);
		return this;
	}

	virtual DsqlOnlyStmtNode* copy(thread_db* /*tdbb*/, NodeCopier& /*copier*/) const
	{
		fb_assert(false);
		return NULL;
	}

	const StmtNode* execute(thread_db* /*tdbb*/, jrd_req* /*request*/, ExeState* /*exeState*/) const
	{
		fb_assert(false);
		return NULL;
	}
};


// Add savepoint pair of nodes to statement having error handlers.
class SavepointEncloseNode : public TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_SAVEPOINT_ENCLOSE>
{
public:
	explicit SavepointEncloseNode(MemoryPool& pool, StmtNode* aStmt)
		: TypedNode<DsqlOnlyStmtNode, StmtNode::TYPE_SAVEPOINT_ENCLOSE>(pool),
		  stmt(aStmt)
	{
	}

public:
	static StmtNode* make(MemoryPool& pool, DsqlCompilerScratch* dsqlScratch, StmtNode* node);

public:
	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void genBlr(DsqlCompilerScratch* dsqlScratch);

private:
	NestConst<StmtNode> stmt;
};


struct ScaledNumber
{
	FB_UINT64 number;
	SCHAR scale;
	bool hex;
};


class RowsClause : public Printable
{
public:
	explicit RowsClause(MemoryPool& pool)
		: length(NULL),
		  skip(NULL)
	{
	}

public:
	virtual Firebird::string internalPrint(NodePrinter& printer) const;

public:
	NestConst<ValueExprNode> length;
	NestConst<ValueExprNode> skip;
};


class GeneratorItem : public Printable
{
public:
	GeneratorItem(Firebird::MemoryPool& pool, const Firebird::MetaName& name)
		: id(0), name(pool, name), secName(pool)
	{}

	GeneratorItem& operator=(const GeneratorItem& other)
	{
		id = other.id;
		name = other.name;
		secName = other.secName;
		return *this;
	}

public:
	virtual Firebird::string internalPrint(NodePrinter& printer) const;

public:
	SLONG id;
	Firebird::MetaName name;
	Firebird::MetaName secName;
};

typedef Firebird::Array<StreamType> StreamMap;

// Copy sub expressions (including subqueries).
class SubExprNodeCopier : private StreamMap, public NodeCopier
{
public:
	SubExprNodeCopier(Firebird::MemoryPool& pool, CompilerScratch* aCsb)
		: NodeCopier(pool, aCsb, getBuffer(STREAM_MAP_LENGTH))
	{
		// Initialize the map so all streams initially resolve to the original number.
		// As soon as copy creates new streams, the map is being overwritten.
		for (unsigned i = 0; i < STREAM_MAP_LENGTH; ++i)
			remap[i] = i;
	}
};


} // namespace

#endif // DSQL_NODES_H
