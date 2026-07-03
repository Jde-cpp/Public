#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/awaits/QueryAwait.h>
#include <jde/db/generators/Statement.h>

namespace Jde::DB{
	struct IDataSource;
	struct ΓDB ExecuteAwait : UInt32Await{
		using base=UInt32Await;
		ExecuteAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ds}, _sql{ move(s) }{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->QueryAwait::Task;
	private:
 		sp<IDataSource> _ds;
		Sql _sql;
	};
}