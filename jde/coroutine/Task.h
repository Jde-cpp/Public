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

	Φ NextHandle()noexcept->ClientHandle;
	Φ NextTaskHandle()noexcept->ClientHandle;
	Φ NextTaskPromiseHandle()noexcept->ClientHandle;

	struct ITaskError
	{
		//ITaskError():_taskHandle{NextTaskHandle()}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		//const Handle _taskHandle;
	};

	struct AwaitResult
	{
		//function<void(const void*)> Deleter;
		using UType=void*;//std::unique_ptr<void,decltype(Deleter)>;
		using Value = std::variant<UType,sp<void>,IException*>;
		AwaitResult()=default;
		//AwaitResult( AwaitResult&& x ):_result{ move(x._result) }{ x._result = (void*)nullptr;};
		Τ AwaitResult( up<T> p )noexcept:_result{ p.release() }{}
		explicit AwaitResult( UType p )noexcept:_result{ p }{}
		explicit AwaitResult( up<IException> e )noexcept:_result{move(e)}{};
		AwaitResult( sp<void> p )noexcept:_result{ p }{};
		AwaitResult( Exception&& e )noexcept:_result{ e.Move().release() }{};
		//~AwaitResult(){ ASSERT( _result.index()!=0 || !get<0>(_result) ); }
		//α operator=( AwaitResult&& x )noexcept->AwaitResult&{ _result = move( x._result ); return *this; }
		α Clear()noexcept->void{ _result = UType{}; }
		α HasValue()const noexcept{ return _result.index()==0 && get<0>( _result ); }
		α HasShared()const noexcept{ return _result.index()==1 && get<1>( _result ); }
		α HasError()const noexcept{ return _result.index()==2; }
		α Error()noexcept->up<IException>{ auto p = HasError() ? get<IException*>(_result) : nullptr; ASSERT(p); Clear(); return up<IException>{ p->Move() };  }
		α Uninitialized()const noexcept{ return _result.index()==0 && get<0>(_result)==nullptr; }
		α CheckError( SRCE )noexcept(false)->void;
		ⓣ SP( SRCE )noexcept(false)->sp<T>;
		ⓣ UP( SRCE )noexcept(false)->up<T>;

		Φ CheckUninitialized()noexcept->void;
		//α Error()const noexcept->up<IException>{ return HasError() ? get<1>(_result) : nullptr; }

		α Set( void* p )noexcept->void{ CheckUninitialized(); _result = move(p); }
		α Set( IException&& e )noexcept->void{ CheckUninitialized(); _result = e.Move().release(); }
		α Set( Value&& result )noexcept{ _result = move(result); }
	private:
		Value _result;
	};

	struct Task final //: ITaskError
	{
		//Γ Task()noexcept;
		//Γ Task( const Task& )noexcept;
		//Γ Task( Task&& x )noexcept;
		//Γ ~Task()noexcept;
		using TResult=AwaitResult;
		struct promise_type
		{
			promise_type():_promiseHandle{ NextTaskPromiseHandle() }{}
			Task& get_return_object()noexcept{ return _pReturnObject ? *_pReturnObject : *(_pReturnObject=mu<Task>()); }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			Φ unhandled_exception()noexcept->void;
		private:
			up<Task> _pReturnObject;
			const Handle _promiseHandle;
		};
		α Clear()noexcept->void{ _result.Clear(); }
		α HasResult()const noexcept->bool{ return !_result.Uninitialized(); }
		α Result()noexcept->AwaitResult&{ return _result; }
		α SetResult( IException&& e )noexcept->void{ _result.Set( move(e) ); }
		α SetResult( AwaitResult::Value&& r )noexcept->void{ _result.Set( move(r) ); }
		ⓣ SetResult( up<T>&& x )noexcept{ _result.Set( x.release() ); }
		α SetResult( AwaitResult&& r )noexcept->void{ _result = move( r ); }
		ⓣ SetResult( sp<T> x )noexcept{ _result.Set( x ); }
	private:
		//uint i;
		AwaitResult _result;
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

	ⓣ AwaitResult::UP( SL sl )noexcept(false)->up<T>
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

	ⓣ AwaitResult::SP( const source_location& sl )noexcept(false)->sp<T>
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
}
#undef Φ