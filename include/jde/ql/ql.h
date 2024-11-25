#pragma once
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Statement.h>
#include "types/TableQL.h"
#include "types/MutationQL.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct MutationQL; struct TableQL;
	using RequestQL=std::variant<vector<TableQL>,MutationQL>;

	struct QLAwait final : TAwait<jvalue>{
		QLAwait( TableQL&& ql, UserPK userPK, SRCE )ι:TAwait<jvalue>{sl},_request{vector{move(ql)}}, _userPK{userPK}{}
		QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK userPK, SRCE )ι;
		QLAwait( string query, UserPK userPK, SRCE )ε;
		α Suspend()ι->void override;
		α await_resume()ε->jvalue override;
	private:
		RequestQL _request;
		optional<DB::Statement> _statement;
		UserPK _userPK;
	};
	α Query( const TableQL& table, jobject& jData, UserPK userId )ε->void;
	α SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted=nullopt )ι->optional<DB::Statement>;
	α Query( string query, UserPK userId, SRCE )ε->jobject;
	α Parse( string query )ε->RequestQL;
	α Configure( vector<sp<DB::AppSchema>>&& schemas )ε->void;
}