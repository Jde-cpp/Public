#pragma once
#include "Exports-Executor.h"
#include <jde/Log.h>
#include <jde/blockly/IBlockly.h>
#include <jde/coroutine/Task.h>
#include <jde/markets/types/Tick.h>
#include "types/BTick.h"
#include "types/Proc.h"
#include "types/EventResult.h"

namespace Jde::Markets::OrderManager{ struct Cache; }
namespace Jde::Markets::MBlockly
{
	using namespace Coroutine;
	struct AwaitableData final
	{
		AwaitableData( const optional<ProcTimePoint>& alarm, const BTick& tick, optional<function<bool(const Markets::Tick&)>> tickTrigger, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields, uint index ):
			Alarm{alarm},Tick{tick},TickTrigger{tickTrigger},Order{order}, OrderFields{orderFields}, Status{status}, StatusFields{statusFields}, Index{index}
		{}
		~AwaitableData();
		α IsHandled()const ι->bool{ return _value; }
		α Handle()ι->bool{ return _value.exchange( true ); }

		const optional<ProcTimePoint> Alarm;
		BTick Tick; optional<function<bool(const Markets::Tick&)>> TickTrigger; //Tick::Fields TickFields;
		ProcOrder Order; const MyOrder::Fields OrderFields;
		OrderStatus Status; const OrderStatus::Fields StatusFields;
		const uint Index;
		Coroutine::Handle HTick{0}; Coroutine::Handle HOrder{0}; Coroutine::Handle HAlarm{0};

		std::once_flag SingleEnd;
	private:
		atomic<bool> _value{false};
	};

	struct ΓBE EventManager final
	{
		Ω Wait( const optional<ProcTimePoint>& alarm, const BTick& tick, optional<function<bool(const Markets::Tick&)>> tickTrigger, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields )ι->Awaitable;
		Ω LogLevel()ι->const LogTag&;
	};

	template<class T>
	struct BTask final
	{
#define _logLevel EventManager::LogLevel()
		BTask():_taskHandle{NextTaskHandle()}{ TRACE("({})BTask::BTask()", _taskHandle); }
		BTask( const BTask& t2 ):
			Result{t2.Result},
			_taskHandle{t2._taskHandle}
		{
			TRACE( "BTask({})", _taskHandle );
		}
		~BTask(){ TRACE( "({})BTask::~BTask", _taskHandle ); }

		struct promise_type
		{
			promise_type():_promiseHandle{ NextTaskPromiseHandle() }
			{
				TRACE( "({})BTask::promise_type)", _promiseHandle );
			}
#undef _logLevel
			α get_return_object()ι->BTask<T>&{ return _returnObject; }
			suspend_never initial_suspend()ι{ return {}; }
			suspend_never final_suspend()ι{ return {}; }
			α return_void()ι->void{}
			α unhandled_exception()ι->void{ ERR("unhandled_exception");  }
			BTask<T> _returnObject;
			const Handle _promiseHandle;
		};
		T Result;
		const Handle _taskHandle;
	};

	struct ΓBE Awaitable final : std::enable_shared_from_this<Awaitable>
	{
		Awaitable( const optional<ProcTimePoint>& alarm, const BTick& tick, optional<function<bool(const Markets::Tick&)>> tickTrigger, const ProcOrder& order, MyOrder::Fields orderFields, const OrderStatus& status, OrderStatus::Fields statusFields )ι;
		using TReturn=EventResult;
		using TTask=BTask<TReturn>;
		~Awaitable();
		α await_ready()ι->bool;
		α await_suspend( coroutine_handle<TTask::promise_type> h )ι->void; //if( !await_ready){ await_suspend();} await_resume()
		α await_resume()ι->TReturn;
	private:
		optional<std::exception_ptr> _pError;
		coroutine_handle<TTask::promise_type> _taskHandle;

		α Complete( sp<AwaitableData> p, uint type, sp<IException> pError )ι->void;
		α CallAlarm( sp<AwaitableData> pData )ι->Task;
		α CallTick( sp<AwaitableData> pData )ι->Task;
		α CallOrder( sp<AwaitableData> pData )ι->Task;

		sp<AwaitableData> _pData;
	};
}