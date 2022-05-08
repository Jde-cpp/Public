#pragma once

#include <variant>
#include <jde/Log.h>
#include <jde/Exception.h>
#include <jde/Assert.h>

#define Φ Γ auto
namespace Jde::Coroutine
{
	typedef uint Handle;
	typedef Handle ClientHandle;

	Φ NextHandle()ι->ClientHandle;
	Φ NextTaskHandle()ι->ClientHandle;
	Φ NextTaskPromiseHandle()ι->ClientHandle;

	struct AwaitResult
	{
		using UType=void*;//std::unique_ptr<void,decltype(Deleter)>;
		using Value = std::variant<UType,sp<void>,IException*,bool>;
		AwaitResult()=default;
		Τ AwaitResult( up<T> p )ι:_result{ p.release() }{}
		explicit AwaitResult( UType p )ι:_result{ p }{}
		explicit AwaitResult( up<IException> e )ι:_result{move(e)}{};
		AwaitResult( sp<void>&& p )ι:_result{ p }{};
		AwaitResult( Exception&& e )ι:_result{ e.Move().release() }{};
		α Clear()ι->void{ _result = UType{}; }
		α HasValue()Ι{ return _result.index()==0 && get<0>( _result ); }
		α HasShared()Ι{ return _result.index()==1 && get<1>( _result ); }
		α HasError()Ι{ return _result.index()==2; }
		α HasBool()Ι{ return _result.index()==3; }
		α Error()ι->up<IException>{ auto p = HasError() ? get<IException*>(_result) : nullptr; ASSERT(p); Clear(); return up<IException>{ p->Move() };  }
		α Uninitialized()Ι{ return _result.index()==0 && get<0>(_result)==nullptr; }
		α CheckError( SRCE )ε->void;
		ⓣ SP( SRCE )ε->sp<T>;
		ⓣ UP( SRCE )ε->up<T>;
		α Bool( SRCE )ε->bool;
		Φ CheckUninitialized()ι->void;

		α Set( void* p )ι->void{ CheckUninitialized(); _result = move(p); }
		α Set( IException&& e )ι->void{ CheckUninitialized(); _result = e.Move().release(); }
		α Set( Value&& result )ι{ _result = move(result); }
		α SetBool( bool x )ι{ _result = x; }
	private:
		Value _result;
	};

	template<class T> concept IsPolymorphic = std::is_polymorphic_v<T>;

	struct Task final
	{
		using TResult=AwaitResult;
		struct promise_type
		{
			promise_type():_promiseHandle{ NextTaskPromiseHandle() }{ /*TRACE("({:x})promise_type()", (uint)this);*/ }

			α get_return_object()ι->Task&{ return _pReturnObject ? *_pReturnObject : *(_pReturnObject=mu<Task>()); }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι{}
			Φ unhandled_exception()ι->void;
			α SetUnhandledResume( coroutine_handle<Task::promise_type> h )ι{ _unhandledResume=h; /*TRACE( "({:x})SetUnhandledResume({:x})", (uint)this, (uint)h.address() );*/ }
		private:
			up<Task> _pReturnObject;
			const Handle _promiseHandle;
			coroutine_handle<Task::promise_type> _unhandledResume;
		};
		α Clear()ι->void{ _result.Clear(); }
		α HasResult()Ι->bool{ return !_result.Uninitialized(); }
		α HasError()Ι->bool{ return !_result.HasError(); }
		α Result()ι->AwaitResult&{ return _result; }
		α SetResult( IException&& e )ι->void{ _result.Set( move(e) ); }
		α SetResult( AwaitResult::Value&& r )ι->void{ _result.Set( move(r) ); }
		template<IsPolymorphic T> auto SetResult( up<T>&& x )ι{ ASSERT(dynamic_cast<IException*>(x.get())==nullptr); _result.Set( x.release() ); }
		ⓣ SetResult( up<T>&& x )ι{ _result.Set( x.release() ); }
		α SetResult( AwaitResult&& r )ι->void{ _result = move( r ); }
		template<IsPolymorphic T> α SetSP( const sp<T>& x )ι->void{ ASSERT( dynamic_pointer_cast<IException>(x)==nullptr ); _result.Set( x ); }
		ⓣ SetSP( const sp<T>& x )ι{  _result.Set( x ); }
		α SetBool( bool x )ι{ _result.SetBool(x); }
	private:
		AwaitResult _result;
	};
}
namespace Jde
{
	using HCoroutine = coroutine_handle<Coroutine::Task::promise_type>;
	ⓣ SP( Coroutine::AwaitResult& r, HCoroutine h, SRCE )ε->sp<T>;
}

namespace Jde::Coroutine
{
	struct CoException : IException
	{
		CoException( HCoroutine h, IException&& i, SRCE ):IException{sl, ELogLevel::NoLog, {} },_h{h},_pInner{ i.Move() }{}
		α Resume( Task::promise_type& pt )ι{ pt.get_return_object().SetResult( move(*_pInner) ); _h.resume(); }

		α Clone()noexcept->sp<IException> override{ return ms<CoException>(move(*this)); }
		α Move()noexcept->up<IException> override{ return mu<CoException>(move(*this)); }
		α Ptr()->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		HCoroutine _h;
		up<IException> _pInner;
	};

	Ξ AwaitResult::CheckError( SL sl )->void
	{
		if( _result.index()==2 )
		{
			up<IException> pException = Error(); ASSERT( pException );
			pException->Push( sl );
			pException->Throw();
		}
	}

	ⓣ AwaitResult::UP( SL sl )ε->up<T>
	{
		CheckError( sl );
		if( _result.index()==1 )
			throw Exception{ "Result is a shared_ptr.", ELogLevel::Critical, sl };
		void* pUnique = get<0>( _result );
		auto p = static_cast<T*>( pUnique );
		if( pUnique && !p )
			throw Exception{ "Could not cast ptr." };//mysql
		_result = (void*)nullptr;
		return up<T>{ p };
	}

	ⓣ AwaitResult::SP( const source_location& sl )ε->sp<T>
	{
		CheckError( sl );
		if( _result.index()==0 )
			throw Exception{ "Result is a unique_ptr.", ELogLevel::Critical, sl };

		auto pVoid = get<sp<void>>( _result );
		sp<T> p = pVoid ? static_pointer_cast<T>( pVoid ) : sp<T>{};
		if( pVoid && !p )
			throw Exception{ "Could not cast ptr." };//mysql
		return p;
	}
	ⓣ SP( AwaitResult&& r, HCoroutine h, SRCE )ε->sp<T>
	{
		try
		{
			r.SP<T>( sl );
		}
		catch( IException& e )
		{
			throw CoException( h, move(e), sl );
		}
	}
	Ξ AwaitResult::Bool( const source_location& sl )ε->bool
	{
		CheckError( sl );
		if( _result.index()==0 )
			throw Exception{ "Result is a unique_ptr.", ELogLevel::Critical, sl };
		else if( _result.index()==1 )
			throw Exception{ "Result is a shared_ptr.", ELogLevel::Critical, sl };

		return get<bool>( _result );
	}
}
#undef Φ