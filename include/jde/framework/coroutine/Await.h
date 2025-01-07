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
		α Resume()ι{ ASSERT(_h); _h.resume(); }
		α Source()ι->SL{ return _sl; }
	protected:
		α SetError( IException&& e )ι{ ASSERT(Promise()); Promise()->SetError( move(e) ); }
		β Suspend()ι->void{};
		α AwaitResume()ε->void{ if( up<IException> e = Promise() ? Promise()->MoveError() : nullptr; e ) e->Throw(); }
		Handle _h{};
		TPromise* Promise(){ return _h ? &_h.promise() : nullptr; }
		SL _sl;
	};

	template<class Result,class TTask=Jde::TTask<Result>>
	struct TAwait : VoidAwait<Result,TTask>{
		using base = VoidAwait<Result,TTask>;
		TAwait( SRCE )ι:base{sl}{}
		virtual ~TAwait()=0;
		α await_resume()ε->Result;

		α Emplaced()ι{ return base::Promise() && base::Promise()->Emplaced(); }
		α SetValue( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->SetValue( move(r) ); }
		α Resume()ι{ ASSERT(base::Promise()); base::_h.resume(); }
		α Resume( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( std::move(r), base::_h ); }
		α ResumeScaler( Result r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( move(r), base::_h ); }
	};
	template<class Result,class TTask>
	TAwait<Result,TTask>::~TAwait(){};

	template<class Result,class TTask>
	α TAwait<Result,TTask>::await_resume()ε->Result{
		base::AwaitResume();
		if( !base::Promise() )
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "promise is null" };
		if( !base::Promise()->Value() )
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "Value is null" };
		return std::move( *base::Promise()->Value() );
	}

	template<class Result,class TExecuteResult=void, class TTask=Jde::TTask<Result>>
	struct TAwaitEx : TAwait<Result,TTask>{
		using TBase = TAwait<Result,TTask>;
		TAwaitEx( SRCE )ι:TBase{sl}{}
		α Suspend()ι->void override{ Execute(); }
		β Execute()ι->TExecuteResult=0;
	};

	template<class TAwait>
	α BlockVoidAwaitExecute( TAwait&& a, up<IException>& e, atomic_flag& done )ι->TAwait::Task{
		try{
			co_await a;
		}
		catch( IException& e2 ){
			e = e2.Move();
		}
		done.test_and_set();
		done.notify_all();
	}

	template<class TAwait>
	α BlockVoidAwait( TAwait&& a )ε->void{
		atomic_flag done;
		up<IException> e;
		BlockVoidAwaitExecute( move(a), e, done );
		done.wait( false );
		if( e )
			e->Throw();
	}

	template<class TAwait, class TResult>
	α BlockAwaitExecute( TAwait& a, optional<TResult>& y, up<IException>& e, atomic_flag& done )ι->TAwait::Task{
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
	α BlockAwait( TAwait& a )ε->TResult{
		atomic_flag done;
		optional<TResult> y; up<IException> e;
		BlockAwaitExecute( a, y, e, done );
		done.wait( false );
		if( e )
			e->Throw();
		return *y;
	}

	template<class TAwait, class TResult>
	α BlockAwait( TAwait&& a )ε->TResult{
		atomic_flag done;
		optional<TResult> y; up<IException> e;
		BlockAwaitExecute( a, y, e, done );
		done.wait( false );
		if( e )
			e->Throw();
		return *y;
	}
}