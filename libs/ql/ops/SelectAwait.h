#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/types/TableQL.h>

namespace Jde::DB{ struct IDataSource; struct Statement; }
namespace Jde::QL{
	struct SelectAwait final: TAwaitEx<jvalue, TAwait<optional<jvalue>>::Task>{
		using base = TAwaitEx<jvalue, TAwait<optional<jvalue>>::Task>;
		using SubTables=flat_map<string,flat_multimap<uint,jobject>>;//tableName,pk, row
		SelectAwait( TableQL qlTable, UserPK executer, bool log, SL sl ): base{ sl }, _executer{executer}, _log{log}, _qlTable{move(qlTable)}{}
		SelectAwait( TableQL qlTable, DB::Statement sql, UserPK executer, bool log, SL sl ): base{ sl }, _executer{executer}, _log{log}, _qlTable{move(qlTable)}, _statement{move(sql)}{}
		α await_ready()ι->bool override;
		α await_resume()ε->jvalue override;
	private:
		α Execute()ι->TAwait<optional<jvalue>>::Task override;
		α Query()ι->void;
		α Query( DB::Statement statement, SubTables subTables )ε->DB::SelectAwait::Task;
		α SelectSubTables( optional<DB::Statement> parentSql, vector<TableQL> tables, sp<DB::Table> parentTable, DB::WhereClause where )->DB::SelectAwait::Task;

		α DS()->DB::IDataSource&{ return *_ds; }
		sp<DB::IDataSource> _ds;
		UserPK _executer;
		bool _log;
		TableQL _qlTable;
		optional<DB::Statement> _statement;
		variant<nullptr_t,jvalue,up<exception>> _result;
	};
}
