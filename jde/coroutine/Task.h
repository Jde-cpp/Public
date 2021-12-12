#pragma once

#include <variant>
#include <jde/Log.h>
#include <jde/Exception.h>
#include <jde/Assert.h>

namespace Jde::Coroutine
{
	typedef uint Handle;
	typedef Handle ClientHandle;

	Γ α NextHandle()noexcept->ClientHandle;
	Γ α NextTaskHandle()noexcept->ClientHandle;
	Γ α NextTaskPromiseHandle()noexcept->ClientHandle;

	struct ITaskError
	{
		ITaskError():_taskHandle{NextTaskHandle()}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		const Handle _taskHandle;
	};

	struct TaskResult
	{
		using TException=IException;
		TaskResult()=default;
		explicit TaskResult( sp<void> p )noexcept:_result{p}{}
		explicit TaskResult( sp<IException> e )noexcept:_result{dynamic_pointer_cast<TException>(e)}{};
		TaskResult( Exception&& e )noexcept:_result{ dynamic_pointer_cast<TException>(std::make_shared<Exception>(move(e))) }{};
		α Clear()noexcept->void{ _result = sp<void>{}; }
		α HasValue()const noexcept{ return _result.index()==0 && get<sp<void>>( _result ); }
		α HasError()const noexcept{ return _result.index()==1; }
		α Uninitialized()const noexcept{ return _result.index()==0 && get<sp<void>>(_result)==nullptr; }
		ⓣ Get( SRCE )const noexcept(false)->sp<T>;
		Γ α CheckUninitialized()noexcept->void;
		α Error()const noexcept->sp<IException>{ return HasError() ? get<sp<TException>>(_result) : nullptr; }

		α Set( sp<void> p )noexcept->void{ CheckUninitialized(); _result = p; }
		//α Set( TException_ptr p )noexcept->void{ CheckUninitialized(); _result = p; }
		//α Set( TException&& e )noexcept->void{ CheckUninitialized(); Set( std::make_exception_ptr(move(e)) ); }
		α Set( Exception&& e )noexcept->void{ CheckUninitialized(); _result = std::dynamic_pointer_cast<TException>( std::make_shared<Exception>(move(e)) ); }
		α Set( std::variant<sp<void>,sp<TException>>&& result )noexcept{ _result = move(result); }
	private:
		std::variant<sp<void>,sp<IException>> _result;
	};
	struct Task2 final : ITaskError
	{
		using TResult=TaskResult;
		struct promise_type
		{
			promise_type():_promiseHandle{ NextTaskPromiseHandle() }{}
			Task2& get_return_object()noexcept{ return _pReturnObject ? *_pReturnObject : *(_pReturnObject=make_unique<Task2>()); }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			Γ α unhandled_exception()noexcept->void;
		private:
			up<Task2> _pReturnObject;
			const Handle _promiseHandle;
		};
		α Clear()noexcept->void{ Result.Clear(); }
		α HasResult()const noexcept->bool{ return !Result.Uninitialized(); }
		α Get( SL sl )const noexcept(false)->sp<void>{ return GetResult().Get<void>( sl ); }
		α GetResult()const noexcept->const TaskResult&{ return Result; }
		//α SetResult( std::exception_ptr p )noexcept->void{ Result.Set( p ); }
		//α SetResult( std::exception&& e )noexcept->void{ Result.Set( move(e) ); }
		α SetResult( Exception&& e )noexcept->void{ Result.Set( move(e) ); }
		α SetResult( TaskResult&& r )noexcept->void{ Result = move(r); }
		α SetResult( std::variant<sp<void>,sp<TaskResult::TException>>&& r )noexcept{ Result.Set( move(r) ); }
	private:
		TaskResult Result;
	};

	ⓣ TaskResult::Get( const source_location& sl )const noexcept(false)->sp<T>
	{
		if( sp<IException> pException = Error(); pException )
		{
			pException->Push( sl );
			pException->Throw();
		}
		auto pVoid = get<sp<void>>( _result );
		sp<T> p = pVoid ? static_pointer_cast<T>( pVoid ) : sp<T>{};
		if( pVoid && !p ) throw Exception{ "Could not cast ptr.", ELogLevel::Debug, sl };
		return p;
	}
}
