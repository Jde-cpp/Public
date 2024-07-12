﻿#pragma once
#ifndef TASK_H
#define TASK_H
#include <variant>
#include <jde/log/Log.h>
#include <jde/Exception.h>
#include <jde/Assert.h>

#define Φ Γ auto
namespace Jde::Coroutine{
	typedef uint Handle;
	typedef Handle ClientHandle;

	Φ NextHandle()ι->ClientHandle;
	Φ NextTaskHandle()ι->ClientHandle;
	Φ NextTaskPromiseHandle()ι->ClientHandle;

	struct AwaitResult{
		using UType=void*;//std::unique_ptr<void,decltype(Deleter)>;
		using Value = std::variant<UType,sp<void>,IException*,bool>;
		AwaitResult()=default;
		Τ AwaitResult( up<T> p )ι:_result{ p.release() }{}
		explicit AwaitResult( UType p )ι:_result{ p }{}
		explicit AwaitResult( up<IException> e )ι:_result{e.release()}{};
		AwaitResult( sp<void>&& p )ι:_result{ p }{};
		AwaitResult( Exception&& e )ι:_result{ e.Move().release() }{};
		AwaitResult( bool b )ι:_result{ b }{};
		~AwaitResult();
		α Clear()ι->void{ _result = UType{}; }
		α HasValue()Ι{ return _result.index()==0 && get<0>( _result ); }
		α HasShared()Ι{ return _result.index()==1 && get<1>( _result ); }
		α HasError()Ι{ return _result.index()==2; }
		α HasBool()Ι{ return _result.index()==3; }
		α Error()ι->up<IException>{ auto p = HasError() ? get<IException*>(_result) : nullptr; ASSERT(p); Clear(); return p ? up<IException>{ p } : mu<Exception>("nullptr"); }
		α Uninitialized()Ι{ return _result.index()==0 && get<0>(_result)==nullptr; }
		α CheckError( SRCE )ε->void;
		Ŧ SP( SRCE )ε->sp<T>;
		Ŧ UP( SRCE )ε->up<T>;
		α Bool( SRCE )ε->bool;
		Φ CheckUninitialized()ι->void;

		α Set( void* p )ι->void{ CheckUninitialized(); _result = move(p); }
		α Set( IException&& e )ι->void{ CheckUninitialized(); _result = e.Move().release(); }
		α Set( Value&& result )ι{ _result = move(result); }
		α SetBool( bool x )ι{ _result = x; }
		α Push( SL& sl )ι->void{
			if( auto p = _result.index()==2 ? get<2>(_result) : nullptr; p )
				p->Push( sl );
		}
	private:
		Value _result;
	};
	template<class TTask,class TResult>
	struct IPromise{
		α get_return_object()ι->TTask{ return {}; }
		suspend_never initial_suspend()ι{ return {}; }
		suspend_never final_suspend()ι{ return {}; }
		α return_void()ι->void{}
		α unhandled_exception()ι->void;
		β Error()ι->up<IException> =0;
		β SetError( up<IException>&& x )ι->void =0;
	protected:
		TResult Expected;
	};

	template<class TTask,class TResult,class TError=up<IException>>
	struct IExpectedPromise : IPromise<TTask,variant<up<IException>,TResult>>{
		using base = IPromise<TTask,variant<up<IException>,TResult>>;
		using TExpected=variant<TError,TResult>;
		α Error()ι->up<IException> override{ return base::Expected.index()==0 ? move(std::get<0>(base::Expected)) : up<IException>{}; }
		α SetError( up<IException>&& x )ι->void override{ base::Expected = move(x); }
		α Value()ι->TResult*{ return base::Expected.index()==1 ? &std::get<1>(base::Expected) : nullptr; }
		α SetValue( TResult&& x )ι->void{ base::Expected = move(x); }
	};


	struct VoidTask{//TODO move to jde ns
		struct promise_type : IPromise<VoidTask,up<IException>>{
			using base = IPromise<VoidTask,up<IException>>;
			α Error()ι->up<IException> override{ return move(base::Expected); }
			α SetError( up<IException>&& x )ι->void override{ base::Expected = move(x); }
		};
	};

	template<class TResult>
	struct TTask final{
		using Expected=variant<up<IException>,TResult>;
		struct promise_type : IExpectedPromise<TTask<TResult>, TResult, up<IException>>
		{};
	};

	template<class T> concept IsPolymorphic = std::is_polymorphic_v<T>;

	struct Task final{
		using TResult=AwaitResult;
		struct promise_type{
			promise_type()ι:_promiseHandle{ NextTaskPromiseHandle() }{}

			α get_return_object()ι->Task{ return {}; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			Φ unhandled_exception()ι->void;

			α SetResult( IException&& e )ι->void{ Result().Set( move(e) ); }
			Ŧ SetResult( up<T>&& x )ι{ Result().Set( x.release() ); }
			Ŧ SetResult( sp<T>&& x )ι{ Result().Set( move(x) ); }
			α SetResultBool( bool x )ι{ Result().SetBool( x ); }
			template<IsPolymorphic T> α SetResult( up<T>&& x )ι{ ASSERT(dynamic_cast<IException*>(x.get())==nullptr); Result().Set( x.release() ); }
			template<IsPolymorphic T> α SetResult( sp<T>&& x )ι->void{ ASSERT( dynamic_pointer_cast<IException>(x)==nullptr ); Result().Set( move(x) ); }

			α MoveResult()ι->AwaitResult{ if(!_result) return {}; auto y = move(*_result); _result=nullptr; return y;}
			α HasError()Ι->bool{ return HasResult() && _result->HasError(); }
			α HasResult()Ι->bool{ return _result && !_result->Uninitialized(); }
			α Push( SL& sl )ι->void{ Result().Push( sl ); }
		private:
			α Result()ι->AwaitResult&{ if(!_result)_result=mu<AwaitResult>(); return *_result; }
			up<AwaitResult> _result;
			const Handle _promiseHandle;
			source_location _sl;
		};
	};
}
namespace Jde{
	using HCoroutine = coroutine_handle<Coroutine::Task::promise_type>;
	Ŧ Resume( up<T>&& y, HCoroutine h )ι->void{ h.promise().SetResult(move(y)); h.resume(); }
	Ŧ Resume( sp<T>&& y, HCoroutine h )ι->void{ h.promise().SetResult(move(y)); h.resume(); }
	Ξ Resume( IException&& e, HCoroutine h )ι->void{ h.promise().SetResult(move(e)); h.resume(); }
	Ξ ResumeBool( bool y, HCoroutine h )ι->void{ h.promise().SetResultBool(y); h.resume(); }
}

namespace Jde::Coroutine{
	struct CoException final : IException{
		CoException( HCoroutine h, IException&& i, SRCE )ι:IException{sl, ELogLevel::NoLog, "" },_h{h},_pInner{ i.Move() }{}
		α Resume( Task::promise_type& pt )ι{ pt.SetResult( move(*_pInner) ); _h.resume(); }

		α Clone()ι->sp<IException> override{ return ms<CoException>(move(*this)); }
		α Move()ι->up<IException> override{ return mu<CoException>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()ε->void override{ throw move(*this); }
	private:
		HCoroutine _h;
		up<IException> _pInner;
	};

	Ξ AwaitResult::CheckError( SL sl )->void{
		if( _result.index()==2 ){
			up<IException> pException = Error(); ASSERT( pException );
			pException->Push( sl );
			pException->Throw();
		}
	}

	Ŧ AwaitResult::UP( SL sl )ε->up<T>{
		CheckError( sl );
		if( _result.index()==1 )
			throw Exception{ "Result is a shared_ptr.", ELogLevel::Critical, sl };
		else if( _result.index()==3 )
			throw Exception{ "Result is a bool.", ELogLevel::Critical, sl };
		void* pUnique = get<0>( _result );
		auto p = static_cast<T*>( pUnique );
		if( pUnique && !p )
			throw Exception{ "Could not cast ptr." };//mysql
		_result = (void*)nullptr;
		return up<T>{ p };
	}

	Ŧ AwaitResult::SP( const source_location& sl )ε->sp<T>{
		CheckError( sl );
		if( _result.index()==0 )
			throw Exception{ "Result is a unique_ptr.", ELogLevel::Critical, sl };

		auto pVoid = get<sp<void>>( _result );
		sp<T> p = pVoid ? static_pointer_cast<T>( pVoid ) : sp<T>{};
		if( pVoid && !p )
			throw Exception{ "Could not cast ptr." };//mysql
		return p;
	}
	Ŧ SP( AwaitResult&& r, HCoroutine h, SRCE )ε->sp<T>{
		try{
			r.SP<T>( sl );
		}
		catch( IException& e ){
			throw CoException( h, move(e), sl );
		}
	}
	Ξ AwaitResult::Bool( const source_location& sl )ε->bool{
		CheckError( sl );
		if( _result.index()==0 )
			throw Exception{ "Result is a unique_ptr.", ELogLevel::Critical, sl };
		else if( _result.index()==1 )
			throw Exception{ "Result is a shared_ptr.", ELogLevel::Critical, sl };

		return get<bool>( _result );
	}

	template<class TTask,class TExpected>
	α IPromise<TTask,TExpected>::unhandled_exception()ι->void{
		try{
			BREAK;
			throw;
		}
		catch( IException& e ){
			SetError( e.Move() );
			Error()->SetLevel( ELogLevel::Critical );
		}
		catch( nlohmann::json::exception& e ){
			SetError( mu<Jde::Exception>(Jde::Exception{SRCE_CUR, move(e), ELogLevel::Critical, "json exception - {}", e.what()}) );
		}
		catch( std::exception& e ){
			SetError( mu<Jde::Exception>(Jde::Exception{SRCE_CUR, move(e), ELogLevel::Critical, "std::exception - {}", e.what()}) );
		}
		catch( ... ){
			SetError( mu<Jde::Exception>(Jde::Exception{SRCE_CUR, ELogLevel::Critical, "unknown exception"}) );
		}
	}
}
#undef Φ
#endif