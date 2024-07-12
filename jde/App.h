#pragma once
#ifndef APP_H
#define APP_H
#include <jde/Exports.h>
#include "Assert.h"
#include "log/Log.h"

namespace Jde::Threading{ struct InterruptibleThread; struct IWorker; }
#define Φ Γ α
#define ω Γ Ω
namespace Jde{
	Φ AppTag()ι->sp<LogTag>;

	namespace Threading{ struct IPollWorker; }
	struct IShutdown{
		β Shutdown()ι->void=0;
	};

	struct IPollster{
		β WakeUp()ι->void=0;
		β Sleep()ι->void=0;
	};

	struct Γ IApplication{
		Ω Instance()ι->IApplication&{ /*assert(_pInstance);*/ return *_pInstance; }
		α BaseStartup( int argc, char** argv, sv appName, string serviceDescription/*, sv companyName="jde-cpp"*/ )ε->flat_set<string>;
		β Install( str serviceDescription )ε->void=0;
		β Uninstall()ε->void=0;
		Ω EnvironmentVariable( str variable, SRCE )ι->string;

		Ω MemorySize()ι->size_t;
		Ω ExePath()ι->fs::path;
		Ω HostName()ι->string;

		Ω AddThread( sp<Threading::InterruptibleThread> pThread )ι->void;
		Ω RemoveThread( sp<Threading::InterruptibleThread> pThread )ι->void;
		Ω RemoveThread( sv name )ι->sp<Threading::InterruptibleThread>;
		Ω GarbageCollect()ι->void;
		Ω AddApplicationLog( ELogLevel level, str value )ι->void;//static to call in std::terminate.
		Ṫ AddPollster( /*bool appThread*/ )ι->sp<T>;
		Ω AddShutdown( sp<IShutdown> pShared )ι->void;
		Ω RemoveShutdown( sp<IShutdown> pShared )ι->void;
		Ω Add( sp<void> pShared )ι->void;
		Ω Exit( int reason )ι->void;
		Ω Kill( uint processId )ι{return _pInstance ? _pInstance->KillInstance( processId ) : false;}
		Ω Remove( sp<void> pShared )ι->void;
		Ω StartTime()ι->TimePoint;
		Ω AddShutdownFunction( function<void()>&& shutdown )ι->void;
		Ω Pause()ι->int;
		Ω IsConsole()ι->bool;

		Ω GetBackgroundThreads()ι{ return  *_pBackgroundThreads; }
		Ω ApplicationName()ι->sv{ return _pApplicationName ? *_pApplicationName : ""sv;}
		Ω ProgramDataFolder()ι->fs::path;
		Ω ApplicationDataFolder()ι->fs::path;
		Ω ShuttingDown()ι->bool;
		Ω Shutdown( int exitReason )ι->void;
		Ω Cleanup()ι->void;
		Ω AddActiveWorker( Threading::IPollWorker* pWorker )ι->void;
		Ω RemoveActiveWorker( Threading::IPollWorker* p )ι->void;

		constexpr static sv ProductVersion="2024.06.01";
	protected:

		Ω OnTerminate()ι->void;
		β AsService()ι->bool=0;
		β AddSignals()ε->void=0;
		β KillInstance( uint processId )ι->bool=0;

		static mutex _threadMutex;
		static VectorPtr<sp<Threading::InterruptibleThread>> _pBackgroundThreads;

		static sp<IApplication> _pInstance;
		static up<string> _pApplicationName;
	private:
		β SetConsoleTitle( sv title )ι->void=0;

		static vector<sp<void>> _objects; static mutex _objectMutex;
		static vector<Threading::IPollWorker*> _activeWorkers; static std::atomic_flag _activeWorkersMutex;
		static vector<sp<IShutdown>> _shutdowns;
	};

	struct OSApp final: IApplication{
		ω Startup( int argc, char** argv, sv appName, string serviceDescription )ε->flat_set<string>;

		Ω CompanyName()ι->string;
		Ω ProductName()ι->sv;
		Ω SetProductName( sv productName )ι->void;
		Ω CompanyRootDir()ι->fs::path;
		ω FreeLibrary( void* p )ι->void;
		ω LoadLibrary( const fs::path& path )ε->void*;
		ω GetProcAddress( void* pModule, str procName )ε->void*;
		α Install( str serviceDescription )ε->void override;
		α Uninstall()ε->void override;
		ω ProcessId()ι->uint;
		Ω Executable()ι->fs::path;
		Ω Args()ι->const flat_multimap<string,string>&;
		Ω Pause()ι->void;
		Ω UnPause()ι->void;
		Φ GetThreadId()ι->uint;
		Φ GetThreadDescription()ι->const char*;

	protected:
		α KillInstance( uint processId )ι->bool override;
		α SetConsoleTitle( sv title )ι->void override;
		α AddSignals()ε->void override;
		α AsService()ι->bool override;

		//void OnTerminate()ι override;
	private:
		Ω ExitHandler( int s )->void;
#ifdef _MSC_VER
		α HandlerRoutine( DWORD  ctrlType )->BOOL;
#endif
	};

	Ŧ IApplication::AddPollster( /*bool appThread*/ )ι->sp<T>{
		static_assert(std::is_base_of<IShutdown, T>::value, "T must derive from IShutdown");
		static_assert(std::is_base_of<Threading::IPollWorker, T>::value, "T must derive from IPollWorker");
		lg _{ _objectMutex };
		auto p = make_shared<T>();
		_objects.push_back( p );
		_shutdowns.push_back( static_pointer_cast<IShutdown>(p) );
		auto pPoller = static_pointer_cast<Threading::IPollWorker>(p).get();
		if( pPoller->ThreadCount==0 )
			AddActiveWorker( pPoller );
		else
			pPoller->StartThread();

		return p;
	}
}
#undef Φ
#undef ω
#endif