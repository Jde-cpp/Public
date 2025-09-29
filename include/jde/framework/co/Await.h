#pragma once
#ifndef AWAIT_H
#define AWAIT_H
#include "Task.h"

namespace Jde{
	struct VoidAwait{
		using TPromise = VoidTask::promise_type;
		using Task=VoidTask;
		using Handle=coroutine_handle<TPromise>;
		VoidAwait( SRCE )ι:_sl{sl}{}
		β await_ready()ι->bool{ return false; }
		α await_suspend( Handle h )ι->void{ _h=h; Suspend(); }
		β await_resume()ε->void{ AwaitResume(); }
		α ResumeExp( IException&& e )ι{
			ASSERT( Promise() );
			Promise()->ResumeExp( move(e), _h );
		}
		α ResumeExp( exception&& e )ι{
			ASSERT( Promise() );
			Promise()->ResumeExp( move(e), _h );
		}
		α Resume()ι{ ASSERT(_h); auto h=_h; _h=nullptr; h.resume(); }
		α Source()ι->SL{ return _sl; }
	protected:
		α SetError( IException&& e )ι{ ASSERT(Promise()); Promise()->SetExp( move(e) ); }
		β Suspend()ι->void=0;
		α AwaitResume()ε->void{
			if( up<IException> e = Promise() ? Promise()->MoveExp() : nullptr; e ){
				_h = nullptr;
				e->Throw();
			}
		}
		Handle _h{};
		α Promise()->TPromise*{ return _h ? &_h.promise() : nullptr; }
		SL _sl;
	};


	template<class TResult,class TTask>
	struct IAwait{
		using TPromise = TTask::promise_type;
		using Task=TTask;
		using Handle=coroutine_handle<TPromise>;
		IAwait( SRCE )ι:_sl{sl}{}
		β await_ready()ι->bool{ return false; }
		α await_suspend( Handle h )ι->void{ _h=h; Suspend(); }
		//β await_resume()ε->TResult{ AwaitResume(); return TResult{}; }
		β await_resume()ε->TResult = 0;
		α ResumeExp( IException&& e )ι{
			ASSERT(Promise());
			Promise()->ResumeExp( move(e), _h );
		}
		α ResumeExp( exception&& e )ι{
			ASSERT( Promise() );
			Promise()->ResumeExp( move(e), _h );
		}
		α Resume()ι{ ASSERT(_h); auto h=_h; _h=nullptr; h.resume(); }
		α Source()ι->SL{ return _sl; }
	protected:
		α SetError( IException&& e )ι{ ASSERT(Promise()); Promise()->SetExp( move(e) ); }
		β Suspend()ι->void{};
		α AwaitResume()ε->void{
			if( up<IException> e = Promise() ? Promise()->MoveExp() : nullptr; e ){
				_h = nullptr;
				e->Throw();
			}
		}
		Handle _h{};
		α Promise()->TPromise*{ return _h ? &_h.promise() : nullptr; }
		SL _sl;
	};

	template<class Result,class TTask=Jde::TTask<Result>>
	struct TAwait : IAwait<Result,TTask>{
		using base = IAwait<Result,TTask>;
		TAwait( SRCE )ι:base{sl}{}
		virtual ~TAwait()=0;
		α await_resume()ε->Result;

		α Emplaced()ι{ return base::Promise() && base::Promise()->Emplaced(); }
		α SetValue( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->SetValue( move(r) ); }
		α Resume()ι{ ASSERT(base::Promise()); base::_h.resume(); }
		α Resume( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( std::move(r), base::_h ); }
		α ResumeScaler( Result r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( std::move(r), base::_h ); } //win CoGuard doesn't have a copy constructor.
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
		Result result = std::move( *base::Promise()->Value() );
		base::_h = nullptr;
		return result;
	}

	template<class Result,class TExecuteResult=void, class TTask=Jde::TTask<Result>>
	struct TAwaitEx : TAwait<Result,TTask>{
		using TBase = TAwait<Result,TTask>;
		TAwaitEx( SRCE )ι:TBase{sl}{}
		α Suspend()ι->void override{ Execute(); }
		β Execute()ι->TExecuteResult=0;
	};

	Ξ BlockVoidAwaitExecute( VoidAwait&& a, up<IException>& e, atomic_flag& done )ι->VoidAwait::Task{
		try{
			co_await a;
		}
		catch( IException& e2 ){
			e = e2.Move();
		}
		done.test_and_set();
		done.notify_all();
	}

	Ξ BlockVoidAwait( VoidAwait&& a )ε->void{
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
	α BlockAwait( TAwait&& a )ε->TResult{
		atomic_flag done;
		optional<TResult> y; up<IException> e;
		BlockAwaitExecute<TAwait,TResult>( a, y, e, done );
		done.wait( false );
		if( e )
			e->Throw();
		return *y;
	}
}
#endif