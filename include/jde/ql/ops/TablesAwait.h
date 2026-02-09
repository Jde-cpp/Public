#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/IQLSession.h>
#include <jde/ql/types/TableQL.h>

namespace Jde::QL{
	struct IQL;
	struct TablesAwait final: TAwaitEx<jvalue, TAwait<jvalue>::Task>{
		using base = TAwaitEx<jvalue, TAwait<jvalue>::Task>;
		TablesAwait( vector<TableQL>&& tables, optional<DB::Statement>&& statement, QL::Creds creds, sp<IQL>&& ql, SL sl ): base{ sl }, _creds{move(creds)}, _ql{move(ql)}, _statement{move(statement)}, _tables{move(tables)}{}
	private:
		α UserPK()ι->UserPK;
		α Execute()ι->TAwait<jvalue>::Task override;
		QL::Creds _creds;
		sp<IQL> _ql;
		optional<DB::Statement> _statement;
		vector<TableQL> _tables;
	};
}