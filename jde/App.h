#pragma once
#include <jde/Exports.h>
#include "Assert.h"
#include "Log.h"

namespace Jde::Threading{ struct InterruptibleThread; struct IWorker; }
#define Φ Γ auto
namespace Jde
{
	namespace Threading{ struct IPollWorker; }
	struct IShutdown
	{
		β Shutdown()noexcept->void=0;
	};
	struct IPollster
	{
		β WakeUp()noexcept->void=0;
		β Sleep()noexcept->void=0;
	};

	struct Γ IApplication //: IPollster
	{
		virtual ~IApplication();
		Ω Instance()noexcept->IApplication&{ /*assert(_pInstance);*/ return *_pInstance; }
		α BaseStartup( int argc, char** argv, sv appName, string serviceDescription/*, sv companyName="jde-cpp"*/ )noexcept(false)->flat_set<string>;
		β Install( str serviceDescription )noexcept(false)->void=0;
		β Uninstall()noexcept(false)->void=0;

		Ω MemorySize()noexcept->size_t;
		Ω Path()noexcept->fs::path;
		Ω HostName()noexcept->string;

		Ω AddThread( sp<Threading::InterruptibleThread> pThread )noexcept->void;
		Ω RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept->void;
		Ω RemoveThread( sv name )noexcept->sp<Threading::InterruptibleThread>;
		Ω GarbageCollect()noexcept->void;
		Ω AddApplicationLog( ELogLevel level, str value )noexcept->void;//static to call in std::terminate.
		ⓣ static AddPollster( bool appThread )noexcept->sp<T>;
		Ω AddShutdown( sp<IShutdown> pShared )noexcept->void;
		Ω RemoveShutdown( sp<IShutdown> pShared )noexcept->void;
		Ω Add( sp<void> pShared )noexcept->void;
		Ω Exit( int reason )noexcept->void;
		Ω Kill( uint processId )noexcept{return _pInstance ? _pInstance->KillInstance( processId ) : false;}
		Ω Remove( sp<void> pShared )noexcept->void;
		Ω CleanUp()noexcept->void;
		Ω StartTime()noexcept->TimePoint;
		Ω AddShutdownFunction( function<void()>&& shutdown )noexcept->void;
		Ω Pause()noexcept->void;
		Ω IsConsole()noexcept->bool;

		Ω GetBackgroundThreads()noexcept{ return  *_pBackgroundThreads; }
		Ω ApplicationName()noexcept{ return _pApplicationName ? *_pApplicationName : ""sv;}
		β ProgramDataFolder()noexcept->fs::path=0;
		Ω ApplicationDataFolder()noexcept->fs::path;
		Ω ShuttingDown()noexcept->bool;
		Ω Shutdown()noexcept->void;
		β GetEnvironmentVariable( sv variable )noexcept->string=0;
		Ω AddActiveWorker( Threading::IPollWorker* pWorker )noexcept->void;
		Ω RemoveActiveWorker( Threading::IPollWorker* p )noexcept->void;
	protected:

		Ω OnTerminate()noexcept->void;
		β AsService()noexcept->bool=0;
		β AddSignals()noexcept(false)->void=0;
		β KillInstance( uint processId )noexcept->bool=0;

		static mutex _threadMutex;
		static VectorPtr<sp<Threading::InterruptibleThread>> _pBackgroundThreads;

		static sp<IApplication> _pInstance;
		static up<string> _pApplicationName;
	private:
		β SetConsoleTitle( sv title )noexcept->void=0;

		static vector<sp<void>> _objects; static mutex _objectMutex;
		static vector<Threading::IPollWorker*> _activeWorkers; static std::atomic_flag _activeWorkersMutex;
		static vector<sp<IShutdown>> _shutdowns;
	};

	struct OSApp : IApplication
	{
		Γ Ω Startup( int argc, char** argv, sv appName, string serviceDescription )noexcept(false)->flat_set<string>;
		α GetEnvironmentVariable( sv variable )noexcept->string override;
		α ProgramDataFolder()noexcept->fs::path override;
		Ω CompanyName()noexcept->string;
		Ω CompanyRootDir()noexcept->fs::path;
		Γ Ω FreeLibrary( void* p )noexcept->void;
		Γ Ω LoadLibrary( path path )noexcept(false)->void*;
		Γ Ω GetProcAddress( void* pModule, str procName )noexcept(false)->void*;
		α Install( str serviceDescription )noexcept(false)->void override;
		α Uninstall()noexcept(false)->void override;
		Γ Ω ProcessId()noexcept->uint;
		Ω Executable()noexcept->fs::path;
		Ω Args()noexcept->flat_map<string,string>;
		Ω Pause()noexcept->void;
		Ω UnPause()noexcept->void;
		Φ GetThreadId()noexcept->uint;
		Φ GetThreadDescription()noexcept->const char*;

	protected:
		α KillInstance( uint processId )noexcept->bool override;
		α SetConsoleTitle( sv title )noexcept->void override;
		α AddSignals()noexcept(false)->void override;
		α AsService()noexcept->bool override;

		//void OnTerminate()noexcept override;
	private:
		Ω ExitHandler( int s )->void;
#ifdef _MSC_VER
		BOOL HandlerRoutine( DWORD  ctrlType );
#endif
	};

	ⓣ IApplication::AddPollster( bool appThread )noexcept->sp<T>
	{
		static_assert(std::is_base_of<IShutdown, T>::value, "T must derive from IShutdown");
		static_assert(std::is_base_of<Threading::IPollWorker, T>::value, "T must derive from IPollWorker");
		lock_guard _{ _objectMutex };
		auto p = make_shared<T>();
		_objects.push_back( p );
		_shutdowns.push_back( static_pointer_cast<IShutdown>(p) );
		auto pPoller = static_pointer_cast<Threading::IPollWorker>(p).get();
		if( appThread )
			AddActiveWorker( pPoller );
		else
			pPoller->StartThread();

		return p;
	}
}
#undef Φ