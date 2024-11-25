#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/generators/Statement.h>
#include "../../../../../Framework/source/coroutine/Coroutine.h"

namespace Jde::DB{
	struct IDataSource;
	struct ExecuteAwait : TAwait<uint>{
		using base=TAwait<uint>;
		ExecuteAwait( sp<IDataSource> ds, Sql&& s, bool isStoredProc, SL sl )ι:
			base{ sl }, _ds{ move(ds) }, _isStoredProc{isStoredProc}, _sql{ move(s) }
		{}
		α Suspend()ι->void override{ Coroutine::CoroutinePool::Resume( _h ); }
		α await_resume()ι->uint override;
	private:
		sp<IDataSource> _ds;
		bool _isStoredProc;
		Sql _sql;
	};
}