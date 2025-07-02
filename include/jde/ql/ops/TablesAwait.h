#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/types/TableQL.h>

namespace Jde::QL{
	struct TablesAwait final: TAwaitEx<jvalue, TAwait<jvalue>::Task>{
		using base = TAwaitEx<jvalue, TAwait<jvalue>::Task>;
		TablesAwait( vector<TableQL>&& tables, optional<DB::Statement>&& statement, UserPK executer, SL sl ): base{ sl }, _executer{executer}, _statement{move(statement)}, _tables{move(tables)}{}
	private:
		α Execute()ι->TAwait<jvalue>::Task override;
		UserPK _executer;
		optional<DB::Statement> _statement;
		vector<TableQL> _tables;
	};
}