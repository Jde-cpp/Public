#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/usings.h>
#include <jde/db/generators/Sql.h>
#include "../exports.h"

namespace Jde::DB{
	struct IDataSource; struct IRow;
	struct ΓDB RowAwait final : TAwait<vector<up<IRow>>>{
		RowAwait( sp<const IDataSource> ds, Sql&& s, SL sl )ι:RowAwait{ds, move(s), false, sl}{}
		RowAwait( sp<const IDataSource> ds, Sql&& s, bool storedProc, SL sl )ι:TAwait{sl},_ds{ds},_sql{move(s)},_storedProc{storedProc}{}
		α Suspend()ι->void override;
		α await_resume()ε->vector<up<IRow>> override;
	private:
		sp<const IDataSource> _ds;
		Sql _sql;
		bool _storedProc;
	};
}