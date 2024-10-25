#pragma once
#ifndef GRAPHQL_H
#define GRAPHQL_H
#include <jde/framework/str.h>
#include <jde/ql/types/TableQL.h>
#include <jde/db/meta/AppSchema.h>

namespace Jde::DB{ struct IDataSource; struct Syntax; struct Column; }
namespace Jde::QL{
	struct MutationQL;  struct Introspection;
/*
	struct GraphQL final{
		GraphQL( jobject* introspection )ι;
	private:
		α QueryTable( const TableQL& table, UserPK userPK, jobject& jData )ε->void;
		α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject;
		α QueryType( const TableQL& typeTable, jobject& jData )ε->void;
		α Mutation( const MutationQL& m, UserPK userPK )ε->uint;

		up<Introspection> _introspection;
	};*/
}
#endif