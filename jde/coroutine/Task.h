#pragma once
#ifndef JDE_TASK
#define JDE_TASK

#include <variant>
#include <jde/Log.h>

namespace Jde::Coroutine
{
	typedef uint Handle;
	typedef Handle ClientHandle;

	JDE_NATIVE_VISIBILITY ClientHandle NextHandle()noexcept;
	JDE_NATIVE_VISIBILITY ClientHandle NextTaskHandle()noexcept;
	JDE_NATIVE_VISIBILITY ClientHandle NextTaskPromiseHandle()noexcept;

	struct TaskVoid final
	{
		struct promise_type//must be promise_type
		{
			TaskVoid get_return_object()noexcept{ return {}; }
			suspend_never initial_suspend()noexcept{ return {}; }
			suspend_never final_suspend()noexcept{ return {}; }
			void return_void()noexcept{}
			void unhandled_exception()noexcept{}
		};
	};

	template<typename T>
	struct Task final
	{
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
		T Result;
		//std::exception_ptr ExceptionPtr;
		const Handle _taskHandle;
	};


	template<typename TTask>
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


	template<typename T>
	struct TaskError final
	{
		typedef std::variant<T,std::exception_ptr> TResult;
		TaskError():_taskHandle{NextTaskHandle()}{ /*DBG("Task::Task({})"sv, _taskHandle);*/ }
		struct promise_type : PromiseType<TaskError<T>>
		{};
		TResult Result;
		const Handle _taskHandle;
	};
}
#endif