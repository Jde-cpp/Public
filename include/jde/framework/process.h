#pragma once
#ifndef JDE_APP_H
#define JDE_APP_H

namespace Jde::Threading{ struct InterruptibleThread; struct IWorker; }
#define Φ Γ α
#define ω Γ Ω
namespace Jde{ struct IShutdown; }
namespace Jde::Process{
	Φ ApplicationName()ι->const string&;
	α AddKeepAlive( sp<void> pShared )ι->void;
	Φ RemoveKeepAlive( sp<void> pShared )ι->void;

	Φ AddShutdown( sp<IShutdown> pShared )ι->void;
	Φ RemoveShutdown( sp<IShutdown> pShared )ι->void;
	Φ AddShutdownFunction( function<void(bool terminating)>&& shutdown )ι->void;
	Φ AddShutdown( IShutdown* pShutdown )ι->void; //global unique ptrs
	Φ RemoveShutdown( IShutdown* pShutdown )ι->void;

	Φ ExitReason()ι->optional<int>;
	Φ SetExitReason( int reason, bool terminate )ι->void;
	Φ Shutdown( int exitReason )ι->void;
	Φ ShuttingDown()ι->bool;
	Φ Finalizing()ι->bool;
	Ŧ AddPollster( /*bool appThread*/ )ι->sp<T>;

	Φ Args()ι->const flat_multimap<string,string>&;
	Φ FindArg( string key )ι->optional<string>;
	Φ IsDebuggerPresent()ι->bool;
}

namespace Jde{
	namespace Threading{ struct IPollWorker; }
	struct IShutdown{
		β Shutdown( bool terminate )ι->void=0;
	};

	struct IPollster{
		β WakeUp()ι->void=0;
		β Sleep()ι->void=0;
	};
}
namespace Jde{
	struct Γ IApplication{
		Ω Instance()ι->IApplication&;
		Ω SetInstance( sp<IApplication> app )ι->void;
		α BaseStartup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console )ε->flat_set<string>;
		β Install( str serviceDescription )ε->void=0;
		β Uninstall()ε->void=0;
		Ω EnvironmentVariable( str variable, SRCE )ι->optional<string>;

		Ω MemorySize()ι->size_t;
		Ω ExePath()ι->fs::path;
		Ω HostName()ι->string;

		Ω AddThread( sp<Threading::InterruptibleThread> pThread )ι->void;
		Ω RemoveThread( sp<Threading::InterruptibleThread> pThread )ι->void;
		Ω RemoveThread( sv name )ι->sp<Threading::InterruptibleThread>;
		Ω AddApplicationLog( ELogLevel level, str value )ι->void;//static to call in std::terminate.

		Ω Kill( uint processId )ι->bool;
		Ω StartTime()ι->TimePoint;
		Ω Pause()ι->int;
		Ω IsConsole()ι->bool;

//		Ω GetBackgroundThreads()ι{ return  *_pBackgroundThreads; }
		Ω ProgramDataFolder()ι->fs::path;
		Ω ApplicationDataFolder()ι->fs::path;
		Ω AddActiveWorker( Threading::IPollWorker* pWorker )ι->void;
		Ω RemoveActiveWorker( Threading::IPollWorker* p )ι->void;

		constexpr static sv ProductVersion="2024.08.01";
	protected:

		Ω OnTerminate()ι->void;
		β AsService()ι->bool=0;
		β AddSignals()ε->void=0;
		β KillInstance( uint processId )ι->bool=0;

	private:
		β SetConsoleTitle( sv title )ι->void=0;

		static vector<Threading::IPollWorker*> _activeWorkers; static std::atomic_flag _activeWorkersMutex;
	};

	struct OSApp final: IApplication{
		ω Startup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console=nullopt )ε->flat_set<string>;

		Ω CompanyName()ι->string;
		Ω ProductName()ι->sv;
		ω SetProductName( sv productName )ι->void;
		Ω CompanyRootDir()ι->fs::path;
		ω FreeLibrary( void* p )ι->void;
		ω LoadLibrary( const fs::path& path )ε->void*;
		ω GetProcAddress( void* pModule, str procName )ε->void*;
		α Install( str serviceDescription )ε->void override;
		α Uninstall()ε->void override;
		ω ProcessId()ι->uint32;
		Ω Executable()ι->fs::path;
		Ω Pause()ι->void;
		ω UnPause()ι->void;
		Φ GetThreadId()ι->uint;
		Φ GetThreadDescription()ι->const char*;

	protected:
		α KillInstance( uint processId )ι->bool override;
		α SetConsoleTitle( sv title )ι->void override;
		α AddSignals()ε->void override;
		α AsService()ι->bool override;

	private:
		Ω ExitHandler( int s )->void;
#ifdef _MSC_VER
		α HandlerRoutine( DWORD  ctrlType )->BOOL;
#endif
	};

	Ŧ Process::AddPollster( /*bool appThread*/ )ι->sp<T>{
		static_assert(std::is_base_of<IShutdown, T>::value, "T must derive from IShutdown");
		static_assert(std::is_base_of<Threading::IPollWorker, T>::value, "T must derive from IPollWorker");
		auto p = make_shared<T>();
		//Process::AddKeepAlive( p );
		AddShutdown( static_pointer_cast<IShutdown>(p) );
		auto pPoller = static_pointer_cast<Threading::IPollWorker>(p).get();
		if( pPoller->ThreadCount==0 )
			IApplication::AddActiveWorker( pPoller );
		else
			pPoller->StartThread();

		return p;
	}
}
#undef Φ
#undef ω
#endif