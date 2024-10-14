#pragma once
#ifndef GRAPHQL_H
#define GRAPHQL_H
#include <variant>
#include <jde/framework/str.h>
#include <jde/ql/TableQL.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/meta/Schema.h>
//#include <jde/db/generators/Syntax.h>
//#include "../um/UM.h"
#include "exports.h"

#define Φ ΓQL auto
namespace Jde::DB{ struct IDataSource; struct Syntax; struct Column; }
namespace Jde::QL{
	struct MutationQL;  struct Introspection;
/*	namespace GraphQL{
		α DataSource()ι->sp<IDataSource>;
	}
	α AppendQLDBSchema( const Schema& schema )ι->void;//database tables
	Φ SetQLDataSource( sp<IDataSource> p )ι->void;
	α ClearQLDataSource()ι->void;
*/
	struct GraphQL final{
		GraphQL( sp<DB::IDataSource> ds, jobject* introspection )ι;
		α Query( sv query, UserPK userId )ε->jobject;
		α CoQuery( string query, UserPK userId, SRCE )ι->Coroutine::TPoolAwait<jobject>;
	private:
		α QueryTable( const TableQL& table, UserPK userPK, jobject& jData )ε->void;
		α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject;
		α QueryType( const TableQL& typeTable, jobject& jData )ε->void;
		α Mutation( const MutationQL& m, UserPK userPK )ε->uint;

		sp<DB::IDataSource> _ds;
		sp<DB::Schema> _schema;
//		const DB::Syntax _syntax;
		up<Introspection> _introspection;
	};
	typedef std::variant<vector<TableQL>,MutationQL> RequestQL;
	Φ AddMutationListener( string tablePrefix, function<void(const MutationQL& m, uint id)> listener )ι->void;
	α ParseQL( sv query )ε->RequestQL;
	α ToJsonName( const DB::Column& c )ε->tuple<string,string>;//memberName, tableName
}
#undef Φ
#endif