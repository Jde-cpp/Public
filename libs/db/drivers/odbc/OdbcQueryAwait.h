#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/awaits/QueryAwait.h>
#include "OdbcDataSource.h"

namespace Jde::DB::Odbc{
	struct OdbcQueryAwait final : TAwait<Result>{
		using base = TAwait<Result>;
		OdbcQueryAwait( sp<OdbcDataSource> ds, Sql&& s, SL sl ):base{ sl }, _ds{ ds }, _sql{ s }{}
		α Suspend()ι->void override;
	private:
		sp<OdbcDataSource> _ds;
		Sql _sql;
	};
}