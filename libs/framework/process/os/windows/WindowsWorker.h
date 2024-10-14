#pragma once
#include "WindowsHandle.h"
#include <jde/coroutine/Task.h>
#include "../../Framework/source/io/DiskWatcher.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include "../../Framework/source/threading/Worker.h"

namespace Jde::Windows
{
	struct CoEvent{
		coroutine_handle<> CoEvent;
		bool Close;
	};

	struct Event : CoEvent{
		HANDLE WindowsEvent;
	};

	struct WindowsWorker /*final*/: std::enable_shared_from_this<WindowsWorker>{
		WindowsWorker( bool runOnMainThread )ι;
		WindowsWorker( Event&& initial )ι;
		virtual ~WindowsWorker(){};
		α Stop()ι->void;
		α SubPush( Event& e )ι->bool;
		α Stopped()ι{ return ((TimePoint)_stop)!=TimePoint{}; }
	protected:
		WindowsWorker()ι;
		α AddWaitRoutine( Event&& e )ι->void;
		α PreLoop()ι{ _coroutines.reserve( MAXIMUM_WAIT_OBJECTS ); _objects.reserve( MAXIMUM_WAIT_OBJECTS ); AddInternalEvents(); }
		α MainLoop()ι->optional<DWORD>;
		α Loop()ι->DWORD;
		β HandleEvent( Event&& e )ι->void;
		HANDLE _eventQueue;
		HANDLE _eventStop;
		sp<WindowsWorker> _pKeepAlive;
		atomic<TimePoint> _stop;
		vector<CoEvent> _coroutines;
		vector<HANDLE> _objects;
		QueueMove<Event> _queue;
	private:
		α AddInternalEvents()ι->void{ _objects.push_back( _eventQueue );  _objects.push_back( _eventStop ); if( _eventWorker ) _objects.push_back( _eventWorker ); }
		constexpr virtual uint MaxEvents()ι{return MAXIMUM_WAIT_OBJECTS-2; }
		β HandleWorkerEvent()ι->void{ ASSERT(false); }
		α IsMainThread()Ι{ return _eventWorker!=nullptr; }
		Duration _shutdownWait{5s};
		HANDLE _eventWorker{ nullptr };
		up<std::jthread> _pThread;//3rd class...
		std::atomic_flag _lock;

	};

	struct WindowsWorkerMain final: WindowsWorker
	{
		Γ Ω Push( coroutine_handle<>&& h, HANDLE hEvent, bool close=true )ι->void;
		Γ Ω Start( optional<bool> service )ι->void;
		Γ Ω Stop( int exitCode )ι->void;
	private:
		WindowsWorkerMain( bool runOnMainThread )ι;
		vector<sp<WindowsWorker>> _workerBuffers;
		constexpr virtual uint MaxEvents()ι override{return MAXIMUM_WAIT_OBJECTS-3; }
		void HandleEvent( Event&& e )ι override;
		void HandleWorkerEvent()ι override;
		static up<WindowsWorkerMain> _pInstance;
	};
}