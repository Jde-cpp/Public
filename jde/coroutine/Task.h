#pragma once
#ifndef JDE_TASK
#define JDE_TASK

#include <variant>
#include <jde/Log.h>
#include <jde/Exception.h>

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
		TaskResult( sp<void> r )noexcept:Ptr{r}{}
		TaskResult( std::exception_ptr e )noexcept:ExceptionPtr{e}{};
		bool HasError()const noexcept{ return ExceptionPtr!=nullptr; }
		ⓣ Get()noexcept(false)
		{
			if( ExceptionPtr )
				 std::rethrow_exception( ExceptionPtr );
			THROW_IF( !Ptr, "No Result" );
			auto p = static_pointer_cast<T>( Ptr );
			THROW_IF( !p, "Could not cast ptr." );
			return p;
		}
		std::exception_ptr ExceptionPtr;
		sp<void> Ptr;
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
		TResult Result;
	};
}
#endif