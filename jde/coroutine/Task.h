#pragma once

#include <variant>
#include <jde/Log.h>
#include <jde/Exception.h>
#include <jde/Assert.h>

namespace Jde::Coroutine
{
	#define 🚪 JDE_NATIVE_VISIBILITY auto
	typedef uint Handle;
	typedef Handle ClientHandle;

	🚪 NextHandle()noexcept->ClientHandle;
	🚪 NextTaskHandle()noexcept->ClientHandle;
	🚪 NextTaskPromiseHandle()noexcept->ClientHandle;

	struct ITaskError
	{
		ITaskError():_taskHandle{NextTaskHandle()}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		const Handle _taskHandle;
	};

	struct TaskResult
	{
		TaskResult()=default;
		explicit TaskResult( sp<void> p )noexcept:_result{p}{}
		TaskResult( std::exception_ptr e )noexcept:_result{e}{};
		TaskResult( Exception&& e )noexcept:_result{ std::make_exception_ptr(move(e)) }{};
		α Clear()noexcept->void{ _result = sp<void>{}; }
		α HasValue()const noexcept{ return _result.index()==0 && get<sp<void>>( _result ); }
		α HasError()const noexcept{ return _result.index()==1; }
		α Uninitialized()const noexcept{ return _result.index()==0 && get<sp<void>>(_result)==nullptr; }
		ⓣ Get()const noexcept(false)->sp<T>;
		🚪 CheckUninitialized()noexcept->void;
		α Error()noexcept->std::exception_ptr{ return HasError() ? get<std::exception_ptr>(_result) : nullptr; }

		α Set( sp<void> p )noexcept->void{ CheckUninitialized(); _result = p; }
		α Set( std::exception_ptr p )noexcept->void{ CheckUninitialized(); _result = p; }
		α Set( Exception&& e )noexcept->void{ CheckUninitialized(); Set( std::make_exception_ptr(move(e)) ); }
		α Set( std::variant<sp<void>,std::exception_ptr>&& result )noexcept{ _result = move(result); }
	private:
		std::variant<sp<void>,std::exception_ptr> _result;
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
			🚪 unhandled_exception()noexcept->void;
		private:
			up<Task2> _pReturnObject;
			const Handle _promiseHandle;
		};
		α Clear()noexcept->void{ Result.Clear(); }
		α HasResult()const noexcept->bool{ return !Result.Uninitialized(); }
		α GetResult()const noexcept->const TaskResult&{ return Result; }
		α SetResult( std::exception_ptr p )noexcept->void{ Result.Set( p ); }
		α SetResult( Exception&& e )noexcept->void{ Result.Set( move(e) ); }
		α SetResult( TaskResult&& r )noexcept->void{ Result = move(r); }
		α SetResult( std::variant<sp<void>,std::exception_ptr>&& r )noexcept{ Result.Set( move(r) ); }
	private:
		TaskResult Result;
	};

	ⓣ TaskResult::Get()const noexcept(false)->sp<T>
	{
		if( HasError() )
				std::rethrow_exception( get<std::exception_ptr>(_result) );
		auto pVoid = get<sp<void>>( _result );
		sp<T> p = pVoid ? static_pointer_cast<T>( pVoid ) : sp<T>{};
		THROW_IF( pVoid && !p, "Could not cast ptr." );
		return p;
	}
}
#undef 🚪