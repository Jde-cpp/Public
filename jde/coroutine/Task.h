#pragma once

#include <variant>
#include <jde/Log.h>
#include <jde/Exception.h>
#include <jde/Assert.h>

namespace Jde::Coroutine
{
	typedef uint Handle;
	typedef Handle ClientHandle;

	JDE_NATIVE_VISIBILITY ClientHandle NextHandle()noexcept;
	JDE_NATIVE_VISIBILITY ClientHandle NextTaskHandle()noexcept;
	JDE_NATIVE_VISIBILITY ClientHandle NextTaskPromiseHandle()noexcept;

struct task{
  struct promise_type {
    task get_return_object() { return {}; }
    suspend_never initial_suspend() { return {}; }
    suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
};
	struct TaskVoid final
	{
		using TResult=void;
		struct promise_type//must be promise_type
		{
			TaskVoid get_return_object()noexcept{ return {}; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{}
		};
	};

	template<class T>
	struct Task final
	{
		using TResult=T;
		Task():_taskHandle{NextTaskHandle()}{ DBG("Task::Task({})"sv, _taskHandle); }
		Task( const Task& t2 ):
			Result{t2.Result},
			_taskHandle{t2._taskHandle}
		{
			DBG("Task(Task{})"sv, _taskHandle);
		}
		~Task(){ DBG( "Task::~Task({})"sv, _taskHandle); }

		struct promise_type
		{
			promise_type():_promiseHandle{ NextTaskPromiseHandle() }
			{
				DBG( "promise_type::promise_type({})"sv, _promiseHandle );
			}
			Task<T>& get_return_object()noexcept{ return _returnObject; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{ /*DBG0("unhandled_exception"sv); TODO uncomment*/  }
			Task<T> _returnObject;
			const Handle _promiseHandle;
		};
		TResult Result;
		const Handle _taskHandle;
	};


	template<class TTask>
	struct PromiseType /*notfinal*/
	{
		PromiseType():_promiseHandle{ NextTaskPromiseHandle() }
		{}
		TTask& get_return_object()noexcept{ return _returnObject; }
		suspend_never initial_suspend()noexcept{ return {}; }
		suspend_never final_suspend()noexcept{ return {}; }
		void return_void()noexcept{}
		void unhandled_exception()noexcept{ /*DBG0("unhandled_exception"sv);*/ }
		TTask _returnObject;
		const Handle _promiseHandle;
	};

	struct ITaskError
	{
		ITaskError():_taskHandle{NextTaskHandle()}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		const Handle _taskHandle;
	};

	template<class T>
	struct TaskError final : ITaskError
	{
		using TResult=std::variant<T,std::exception_ptr>;
		struct promise_type : PromiseType<TaskError<T>>{};

		TResult Result;
	};

	struct TaskResult
	{
		TaskResult()=default;
		explicit TaskResult( sp<void> p )noexcept:_result{p}{}
		TaskResult( std::exception_ptr e )noexcept:_result{e}{};
		TaskResult( Exception&& e )noexcept:_result{ std::make_exception_ptr(move(e)) }{};
		//TaskResult( TaskResult&& rhs )noexcept:_result{ move(rhs._result) }{};
		α HasValue()const noexcept{ return _result.index()==0 && get<sp<void>>( _result ); }
		α HasError()const noexcept{ return _result.index()==1; }
		α Uninitialized()const noexcept{ return _result.index()==0 && !get<sp<void>>(_result); }
		ⓣ Get()noexcept(false)
		{
			if( HasError() )
				 std::rethrow_exception( get<std::exception_ptr>(_result) );
			auto pVoid = get<sp<void>>( _result );
			THROW_IF( !pVoid, "No Result" );
			auto p = static_pointer_cast<T>( pVoid );
			THROW_IF( !p, "Could not cast ptr." );
			return p;
		}
//		ⓣ Get<std::exception_ptr>()noexcept{ return get<std::exception_ptr>(_result); }

		α Set( sp<void> p )noexcept->void{ ASSERT(Uninitialized()); _result = p; }
		α Set( std::exception_ptr p )noexcept->void{ ASSERT(Uninitialized()); _result = p; }
		α Set( Exception&& e )noexcept->void{ ASSERT(Uninitialized()); Set( std::make_exception_ptr(move(e)) ); }
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
			void unhandled_exception()noexcept
			{
				try
				{
					auto p = std::current_exception();
					if( p )
						std::rethrow_exception( p );
					else
						ERR( "unhandled_exception - no exception"sv );
				}
				catch( const Exception& e )
				{
					e.Log();
				}
				catch( const std::exception& e )
				{
					ERR( "unhandled_exception ->{}"sv, e.what() );
				}
				catch( ... )
				{
					ERR( "unhandled_exception"sv );
				}
			}
		private:
			up<Task2> _pReturnObject;
			const Handle _promiseHandle;
		};
		α HasResult()const noexcept->bool{ return !Result.Uninitialized(); }
		α GetResult()const noexcept->const TResult&{ return Result; }
		//α SetResult( sp<void> p )noexcept->void{ Result.Set( p ); }
		α SetResult( std::exception_ptr p )noexcept->void{ Result.Set( p ); }
		α SetResult( Exception&& e )noexcept->void{ Result.Set( move(e) ); }
		α SetResult( TaskResult&& r )noexcept->void{ Result = move(r); }
		α SetResult( std::variant<sp<void>,std::exception_ptr>&& r )noexcept{ Result.Set( move(r) ); }
	private:
		TaskResult Result;
	};
}
