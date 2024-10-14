#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/usings.h>
#include <jde/db/generators/Statement.h>

namespace Jde::DB{
	struct IDataSource; struct IRow;
	struct RowAwait final : TAwait<vector<up<IRow>>>{
		RowAwait( sp<const IDataSource> ds, Statement&& s, SL sl )ι:TAwait{sl},_ds{ds},_statement{s}{}
		α Suspend()ι->void override;
		α await_resume()ι->vector<up<IRow>> override;
	private:
		sp<const IDataSource> _ds;
		Statement _statement;
	};
}