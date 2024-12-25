#pragma once
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Statement.h>
#include "types/TableQL.h"
#include "types/MutationQL.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	//struct MutationQL; struct TableQL;
	using RequestQL=std::variant<vector<TableQL>,MutationQL>;
	struct QLAwait final : TAwait<jvalue>{
		QLAwait( TableQL&& ql, UserPK executer, SRCE )ι:TAwait<jvalue>{sl},_request{vector{move(ql)}}, _executer{executer}{}
		QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK executer, SRCE )ι;
		QLAwait( string query, UserPK executer, SRCE )ε;
		α Suspend()ι->void override;
		α await_resume()ε->jvalue override;
	private:
		RequestQL _request;
		optional<DB::Statement> _statement;
		UserPK _executer;
	};

	struct IQL{
		β Query( string query, UserPK executer, SRCE )ε->up<TAwait<jvalue>> =0;
	};
	α Local()ι->sp<IQL>;


	α Query( const TableQL& table, UserPK executer )ε->jvalue;
	α SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted=nullopt )ι->optional<DB::Statement>;
	α Query( string query, UserPK executer, SRCE )ε->jvalue;
	α QueryObject( string query, UserPK executer, SRCE )ε->jobject;
	α QueryArray( string query, UserPK executer, SRCE )ε->jarray;
	α Execute( string query, UserPK executer, SRCE )ε->jobject;
	α Mutation( string m, UserPK executer, SRCE )ε->jobject;
	α Parse( string query )ε->RequestQL;
	α Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void;
}