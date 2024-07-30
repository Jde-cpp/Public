#pragma once
#include "Task.h"

namespace Jde{
	template<class TResult=void,class TTask=VoidTask>
	struct VoidAwait{
		using TPromise = TTask::promise_type;
		using Task=TTask;
		using Handle=coroutine_handle<TPromise>;
		VoidAwait( SRCE )ι:_sl{sl}{}
		β await_ready()ι->bool{ return false; }
		β await_suspend( Handle h )ε->void{ _h=h; }
		β await_resume()ε->TResult{ AwaitResume(); return TResult{}; }
		α Resume()ι{ ASSERT(_h); _h.resume(); }
		α ResumeExp( IException&& e )ι{ ASSERT(Promise()); Promise()->ResumeWithError( move(e), _h ); }
		α SetError( IException&& e )ι{ ASSERT(Promise()); Promise()->SetError( move(e) ); }
	protected:
		α AwaitResume()ε->void{
			if( up<IException> e = Promise() ? Promise()->MoveError() : nullptr; e )
				e->Throw();
		}
		Handle _h{};
		TPromise* Promise(){ return _h ? &_h.promise() : nullptr; }
		SL _sl;
	};

	template<class Result,class TTask=TTask<Result>>
	struct TAwait : VoidAwait<Result,TTask>{
		using base = VoidAwait<Result,TTask>;
		TAwait( SRCE )ι:base{sl}{}
		β await_resume()ε->Result{
			base::AwaitResume();
			if( !base::Promise() )
				throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "promise is null" };
			if( !base::Promise()->Value() )
				throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "Value is null" };
			return move( *base::Promise()->Value() );
		};
		α Emplaced()ι{ return base::Promise() && base::Promise()->Emplaced(); }

		α SetValue( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->SetValue( move(r) ); }
		α Resume()ι{ ASSERT(base::Promise()); base::_h.resume(); }

		α Resume( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( move(r), base::_h ); }
		α ResumeScaler( Result r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( r, base::_h ); }
	};

	template<class Result,class TExecuteResult=void, class TTask=TTask<Result>>
	struct TAwaitEx : TAwait<Result,TTask>{
		using base = TAwait<Result,TTask>;
		TAwaitEx( SRCE )ι:base{sl}{}
		α await_suspand()ε->void{ base::await_suspend(base::_h); Execute(); }
		β Execute()ι->TExecuteResult=0;
	};

	template<class TAwait, class TResult>
	α BlockAwaitExecute( TAwait&& a, optional<TResult>& y, up<IException>& e, atomic_flag& done )ι->TAwait::Task{
		try{
			Trace( ELogTags::App, "BlockAwaitExecute {}", done.test() );
			y = co_await a;
		}
		catch( IException& e2 ){
			e = e2.Move();
		}
		Trace( ELogTags::App, "BlockAwaitExecute2 {}", done.test() );
		done.test_and_set();
		Trace( ELogTags::App, "BlockAwaitExecute3 {}", done.test() );
		done.notify_all();
	}

	template<class TAwait, class TResult>
	α BlockAwait( TAwait&& a )ε->TResult{
		atomic_flag done;
		optional<TResult> y; up<IException> e;
		BlockAwaitExecute( move(a), y, e, done );
		Trace( ELogTags::App, "BlockAwait {}", done.test() );
		done.wait( false );
		Trace( ELogTags::App, "BlockAwait2 {}", done.test() );
		if( e )
			e->Throw();
		return *y;
	}
}