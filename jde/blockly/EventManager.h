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
//	using std::experimental::coroutine_handle;
//	using std::experimental::suspend_never;
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

		//mutex HandleMutex;
		std::once_flag SingleEnd;
	private:
		atomic<bool> _value{false};
	};
	struct JDE_BLOCKLY_EXECUTOR Awaitable final : std::enable_shared_from_this<Awaitable>
	{
		Awaitable( const optional<ProcTimePoint>& alarm, const BTick& tick, const Tick::Fields& tickFields, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields )noexcept;
		typedef EventResult TReturn;
		typedef Coroutine::Task<TReturn> TTask;
		~Awaitable();
		bool await_ready()noexcept;
		void await_suspend( coroutine_handle<TTask::promise_type> h )noexcept; //if( !await_ready){ await_suspend();} await_resume()
		TReturn await_resume()noexcept;
	private:
		/*std::once_flag _singleEnd;*/
		//TTask::promise_type* _promisePtr;
		optional<std::exception_ptr> _pError;
		coroutine_handle<TTask::promise_type> _taskHandle;

		void Complete( sp<AwaitableData> p, uint type, std::exception_ptr* pError )noexcept;
		Coroutine::TaskVoid CallAlarm( sp<AwaitableData> pData )noexcept;
		Coroutine::TaskError<Tick> CallTick( sp<AwaitableData> pData )noexcept;
		Coroutine::Task<OrderManager::Cache> CallOrder( sp<AwaitableData> pData )noexcept;

		sp<AwaitableData> _pData;
	};
	struct JDE_BLOCKLY_EXECUTOR EventManager final
	{
		static Awaitable Wait( const optional<ProcTimePoint>& alarm, const BTick& tick, const Tick::Fields& tickFields, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields )noexcept;
	};
}