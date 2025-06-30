#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/awaits/QueryAwait.h>

namespace Jde::DB{
	struct IDataSource;
	α ΓDB OutExecute( sp<IDataSource>&& _ds, Sql&& _sql, function<void(DB::Value&&)> onResult, function<void(IException&&)> onError, SL sl )ι->QueryAwait::Task;

	Τ struct OutAwait : TAwaitEx<T,QueryAwait::Task>{
		using base=TAwaitEx<T,void>;
		OutAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ds}, _sql{ move(s) }{}
		α Execute()ι->void override;
	private:
 		sp<IDataSource> _ds;
		Sql _sql;
	};

	Ŧ OutAwait<T>::Execute()ι->void{
		OutExecute( move(_ds), move(_sql),
			[&](DB::Value&& v){ base::Resume( move(v.Get<T>()) ); },
			[&](IException&& e){ base::ResumeExp( move(e) ); },
			base::_sl );
	}
}