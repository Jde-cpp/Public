#pragma once
#ifndef AWAIT_H
#define AWAIT_H
#include <condition_variable>
#include "Task.h"

namespace Jde{
	struct VoidAwait{
		using TPromise = VoidTask::promise_type;
		using Task=VoidTask;
		using Handle=coroutine_handle<TPromise>;
		VoidAwait( SRCE )ι:_sl{sl}{}
		β await_ready()ι->bool{ return false; }
		α await_suspend( Handle h )ι->void{ _h=h; Suspend(); }
		//Diagnostic only - a matching caller selects the non-template overload above; a mismatched one lands here and the static_assert names the rule instead of 'no viable conversion from coroutine_handle<...>'.
		Τ α await_suspend( coroutine_handle<T> )ι->void{
			static_assert( std::is_same_v<T,TPromise>, "An awaitable dictates its caller's return type: a coroutine may only co_await awaitables whose ::Task is its own return type (CLAUDE.md 'Coroutines'). Split into a hand-off chain, or co_await Any(awaitable) from co/AnyAwait.h." );
		}
		β await_resume()ε->void{ AwaitResume(); }
		α ResumeExp( Exception&& e )ι{
			ASSERT( Promise() );
			Promise()->ResumeExp( move(e), _h );
		}
		α ResumeExp( runtime_error&& e )ι{
			ASSERT( Promise() );
			Promise()->ResumeExp( move(e), _h );
		}
		α Resume()ι{ ASSERT(_h); auto h=_h; _h=nullptr; h.resume(); }
		α Source()ι->SL{ return _sl; }
	protected:
		α SetError( Exception&& e )ι{ ASSERT(Promise()); Promise()->SetExp( move(e) ); }
		β Suspend()ι->void=0;
		α AwaitResume()ε->void{
			if( up<Exception> e = Promise() ? Promise()->MoveExp() : nullptr; e ){
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
		Ξ await_suspend( Handle h )ι->void{ _h=h; Suspend(); }
		//Diagnostic only - a matching caller selects the non-template overload above; a mismatched one lands here and the static_assert names the rule instead of 'no viable conversion from coroutine_handle<...>'.
		Τ Ξ await_suspend( coroutine_handle<T> )ι->void{
			static_assert( std::is_same_v<T,TPromise>, "An awaitable dictates its caller's return type: a coroutine may only co_await awaitables whose ::Task is its own return type (CLAUDE.md 'Coroutines'). Split into a hand-off chain, or co_await Any(awaitable) from co/AnyAwait.h." );
		}
		β await_resume()ε->TResult = 0;
		α ResumeExp( Exception&& e )ι{
			ASSERT(Promise());
			Promise()->ResumeExp( move(e), _h );
		}
		α ResumeExp( runtime_error&& e )ι{
			ASSERT( Promise() );
			Promise()->ResumeExp( move(e), _h );
		}
		α Resume()ι{ ASSERT(_h); auto h=_h; _h=nullptr; h.resume(); }
		α Source()ι->SL{ return _sl; }
	protected:
		α SetError( Exception&& e )ι{ ASSERT(Promise()); Promise()->SetExp( move(e) ); }
		β Suspend()ι->void{};
		α CheckException()ε->void{
			if( up<Exception> e = Promise() ? Promise()->MoveExp() : nullptr; e ){
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
		α Resume( Result&& r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( std::move(r), base::_h ); }
		α ResumeScaler( Result r )ι{ ASSERT(base::Promise()); base::Promise()->Resume( std::move(r), base::_h ); } //win CoGuard doesn't have a copy constructor.
	};
	template<class Result,class TTask>
	TAwait<Result,TTask>::~TAwait(){};

	template<class Result,class TTask>
	α TAwait<Result,TTask>::await_resume()ε->Result{
		base::CheckException();
		if( !base::Promise() )
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "promise is null" };
		if( !base::Promise()->Value() )
			throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::Critical, "Value is null" };
		Result result( std::move(*base::Promise()->Value()) );  //don't use initializer list.
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

	//msvc multiple defined symbols without
	struct Γ StringAwait : TAwait<string>{
		StringAwait( SRCE )ι:TAwait<string>{ sl }{}
	};

	struct Γ UInt32Await : TAwait<uint32>{
		UInt32Await( SRCE )ι:TAwait<uint32>{ sl }{}
	};


	//The blocking half of the bridge.  Wait() never gives up - a bounded wait would turn every legitimately long sync call
	//(schema sync, LocalQL::Upsert) into a spurious failure - but it warns on an interval naming the awaitable's source
	//location, so a response that is dropped instead of delivered leaves a trail rather than 7 minutes of silence.
	//reviews/gateway-review.md #36.
	struct Γ BlockAwaitSync{
		α Signal()ι->void;
		α Wait( SL sl )ι->void;
	private:
		std::mutex _mutex;
		std::condition_variable _cv;
		bool _done{};
	};

	//shared between the blocked thread & the coroutine: the waiter can wake & return between the signal and the notify,
	//so stack-owned state would be destroyed while notify_all still touches it - the coroutine frame co-owns it instead.
	template<class TResult>
	struct BlockAwaitState : BlockAwaitSync{
		optional<TResult> Result;
		up<Exception> Error;
	};

	Ξ BlockVoidAwaitExecute( VoidAwait&& a, sp<BlockAwaitState<std::monostate>> s )ι->VoidAwait::Task{
		try{
			co_await a;
		}
		catch( Exception& e2 ){
			s->Error = e2.Move();
		}
		s->Signal();
	}

	Ξ BlockVoidAwait( VoidAwait&& a )ε->void{
		auto s = ms<BlockAwaitState<std::monostate>>();
		const auto sl = a.Source();//copied before the move - the awaitable is the caller's only clue to what a stalled wait is waiting on.
		BlockVoidAwaitExecute( move(a), s );
		s->Wait( sl );
		if( s->Error )
			s->Error->Throw();
	}

	template<class TAwait, class TResult>
	α BlockAwaitExecute( TAwait& a, sp<BlockAwaitState<TResult>> s )ι->TAwait::Task{
		try{
			s->Result = co_await a;
		}
		catch( Exception& e2 ){
			s->Error = e2.Move();
		}
		s->Signal();
	}

	template<class TAwait, class TResult>
	α BlockAwait( TAwait&& a )ε->TResult{
		auto s = ms<BlockAwaitState<TResult>>();
		const auto sl = a.Source();
		BlockAwaitExecute<TAwait,TResult>( a, s );
		s->Wait( sl );
		if( s->Error )
			s->Error->Throw();
		return move( *s->Result );
	}

	Ŧ BlockTAwait( TAwait<T>&& a )ε->T{
		return BlockAwait<TAwait<T>,T>( move(a) );
	}
}
#endif