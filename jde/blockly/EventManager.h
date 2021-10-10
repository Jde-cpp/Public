#pragma once
#include <jde/coroutine/Task.h>
#include <jde/markets/types/Tick.h>
#include <jde/Log.h>
#include "types/BTick.h"
#include "types/Proc.h"
#include "types/EventResult.h"
#include <jde/blockly/IBlockly.h>

#include "Exports-Executor.h"

namespace Jde::Markets::OrderManager{ struct Cache; }
namespace Jde::Markets::MBlockly
{
	using namespace Coroutine;
	struct AwaitableData final
	{
		AwaitableData( const optional<ProcTimePoint>& alarm, const BTick& tick, const Tick::Fields& tickFields, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields, uint index ):
			Alarm{alarm},Tick{tick},TickFields{tickFields},Order{order}, OrderFields{orderFields}, Status{status}, StatusFields{statusFields}, Index{index}
		{}
		~AwaitableData(){ DBG("~AwaitableData({})"sv, Index); }
		bool IsHandled()const noexcept{ return _value; }
		bool Handle()noexcept{ return _value.exchange( true ); }

		const optional<ProcTimePoint> Alarm;
		BTick Tick; Tick::Fields TickFields;
		ProcOrder Order; const MyOrder::Fields OrderFields;
		OrderStatus Status; const OrderStatus::Fields StatusFields;
		const uint Index;
		Coroutine::Handle HTick{0}; Coroutine::Handle HOrder{0}; Coroutine::Handle HAlarm{0};

		std::once_flag SingleEnd;
	private:
		atomic<bool> _value{false};
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

	struct JDE_BLOCKLY_EXECUTOR Awaitable final : std::enable_shared_from_this<Awaitable>
	{
		Awaitable( const optional<ProcTimePoint>& alarm, const BTick& tick, const Tick::Fields& tickFields, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields )noexcept;
		typedef EventResult TReturn;
		typedef Task<TReturn> TTask;
		~Awaitable();
		bool await_ready()noexcept;
		void await_suspend( coroutine_handle<TTask::promise_type> h )noexcept; //if( !await_ready){ await_suspend();} await_resume()
		TReturn await_resume()noexcept;
	private:
		optional<std::exception_ptr> _pError;
		coroutine_handle<TTask::promise_type> _taskHandle;

		void Complete( sp<AwaitableData> p, uint type, std::exception_ptr pError )noexcept;
		Task2 CallAlarm( sp<AwaitableData> pData )noexcept;
		Task2 CallTick( sp<AwaitableData> pData )noexcept;
		Task2 CallOrder( sp<AwaitableData> pData )noexcept;

		sp<AwaitableData> _pData;
	};
	struct JDE_BLOCKLY_EXECUTOR EventManager final
	{
		static Awaitable Wait( const optional<ProcTimePoint>& alarm, const BTick& tick, const Tick::Fields& tickFields, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields )noexcept;
	};
}