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
		α await_suspend( Handle h )ι->void{ _h=h; Suspend(); }  //msvc internal compiler error if virtual.
		β await_resume()ε->TResult{ AwaitResume(); return TResult{}; }
		α ResumeExp( IException&& e )ι{ ASSERT(Promise()); Promise()->ResumeWithError( move(e), _h ); }

	protected:
		α Resume()ι{ ASSERT(_h); _h.resume(); }
		α SetError( IException&& e )ι{ ASSERT(Promise()); Promise()->SetError( move(e) ); }
		β Suspend()ι->void=0;
		α AwaitResume()ε->void{ if( up<IException> e = Promise() ? Promise()->MoveError() : nullptr; e ) e->Throw(); }
		Handle _h{};
		TPromise* Promise(){ return _h ? &_h.promise() : nullptr; }
		SL _sl;
	};

	template<class Result,class TTask=Jde::TTask<Result>>
	struct TAwait : VoidAwait<Result,TTask>{
		using base2 = VoidAwait<Result,TTask>;
		TAwait( SRCE )ι:base2{sl}{}
		virtual ~TAwait()=0;
		α await_resume()ε->Result;

		α Emplaced()ι{ return base2::Promise() && base2::Promise()->Emplaced(); }
		α SetValue( Result&& r )ι{ ASSERT(base2::Promise()); base2::Promise()->SetValue( move(r) ); }
		α Resume()ι{ ASSERT(base2::Promise()); base2::_h.resume(); }
		α Resume( Result&& r )ι{ ASSERT(base2::Promise()); base2::Promise()->Resume( std::move(r), base2::_h ); }
		α ResumeScaler( Result r )ι{ ASSERT(base2::Promise()); base2::Promise()->Resume( move(r), base2::_h ); }
	};
	template<class Result,class TTask>
	TAwait<Result,TTask>::~TAwait(){};

	template<class Result,class TTask>
	α TAwait<Result,TTask>::await_resume()ε->Result{
		base2::AwaitResume();
		if( !base2::Promise() )
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "promise is null" };
		if( !base2::Promise()->Value() )
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "Value is null" };
		return std::move( *base2::Promise()->Value() );
	}

	template<class Result,class TExecuteResult=void, class TTask=Jde::TTask<Result>>
	struct TAwaitEx : TAwait<Result,TTask>{
		using base1 = TAwait<Result,TTask>;
		TAwaitEx( SRCE )ι:base1{sl}{}
		α Suspend()ι->void override{ Execute(); }
		β Execute()ι->TExecuteResult=0;
	};

	template<class TAwait, class TResult>
	α BlockAwaitExecute( TAwait&& a, optional<TResult>& y, up<IException>& e, atomic_flag& done )ι->TAwait::Task{
		try{
			y = co_await a;
		}
		catch( IException& e2 ){
			e = e2.Move();
		}
		done.test_and_set();
		done.notify_all();
	}

	template<class TAwait, class TResult>
	α BlockAwait( TAwait&& a )ε->TResult{
		atomic_flag done;
		optional<TResult> y; up<IException> e;
		BlockAwaitExecute( move(a), y, e, done );
		done.wait( false );
		if( e )
			e->Throw();
		return *y;
	}
}