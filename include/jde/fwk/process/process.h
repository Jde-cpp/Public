#pragma once
#ifndef JDE_APP_H
#define JDE_APP_H

namespace Jde::Threading{ struct InterruptibleThread; struct IWorker; }
#define Φ Γ α
namespace Jde{ struct IShutdown; }
namespace Jde::Process{
	Φ ApplicationName()ι->const string&;
	Φ ApplicationDataFolder()ι->fs::path;
	Φ Args()ι->const flat_multimap<string,string>&;
	Φ FindArg( string key )ι->optional<string>;
	Φ CompanyName()ι->string;
	Φ CompanyRootDir()ι->fs::path;
	Φ EnvironmentVariable( str variable, SRCE )ι->optional<string>;
	Φ Executable()ι->fs::path;
	Φ ExePath()ι->fs::path;
	Φ HostName()ι->string;
	Φ IsConsole()ι->bool;
	Φ IsDebuggerPresent()ι->bool;
	Φ SetConsole( bool isConsole )ι->void;
	Φ SetConsoleTitle( sv title )ι->void;
	Φ MemorySize()ι->size_t;
	Φ ProcessId()ι->uint32;
	constexpr static sv ProductVersion="2025.10.01";
	Φ ProgramDataFolder()ι->fs::path;
	Φ ProductName()ι->sv;
	Φ StartTime()ι->TimePoint;

	Φ Startup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console=nullopt )ε->flat_set<string>;
	Φ AddSignals()ε->void;
	Φ AsService()ι->bool;
	Φ Pause()ι->int;
	Φ UnPause()ι->void;

	Φ LoadLibrary( const fs::path& path )ε->void*;
	Φ FreeLibrary( void* p )ι->void;
	Φ GetProcAddress( void* pModule, str procName )ε->void*;

	Φ AddApplicationLog( ELogLevel level, str value )ι->void;
	Φ Kill( uint processId )ι->bool;


	// α AddKeepAlive( sp<void> pShared )ι->void;
	// Φ RemoveKeepAlive( sp<void> pShared )ι->void;

	// Φ AddShutdown( sp<IShutdown> pShared )ι->void;
	// Φ RemoveShutdown( sp<IShutdown> pShared )ι->void;
	Φ AddShutdownFunction( function<void(bool terminating)>&& shutdown )ι->void;
	//
	//

	Φ AddShutdown( IShutdown* pShutdown )ι->void; //global unique ptrs
	Φ RemoveShutdown( IShutdown* pShutdown )ι->void;

	Φ ExitException( exception&& e )ι->int;
	Φ ExitHandler( int s )->void;
	Φ ExitReason()ι->optional<int>;
	Φ OnTerminate()ι->void;
	Φ SetExitReason( int reason, bool terminate )ι->void;
	Φ Shutdown( int exitReason )ι->void;
	Φ ShuttingDown()ι->bool;
	Φ Finalizing()ι->bool;
	//Ŧ AddPollster( /*bool appThread*/ )ι->sp<T>;


	Φ Install( str serviceDescription )ε->void;
	Φ Uninstall()ε->void;
}

namespace Jde{
	//namespace Threading{ struct IPollWorker; }
	struct IShutdown{
		β Shutdown( bool terminate )ι->void=0;
	};

/*	struct IPollster{
		β WakeUp()ι->void=0;
		β Sleep()ι->void=0;
	};*/
}
namespace Jde{
/*	struct Γ IApplication{
		//Φ Instance()ι->IApplication&;
		//Φ SetInstance( sp<IApplication> app )ι->void;
		α BaseStartup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console )ε->flat_set<string>;




		//Φ AddThread( sp<Threading::InterruptibleThread> pThread )ι->void;
		//Φ RemoveThread( sp<Threading::InterruptibleThread> pThread )ι->void;
		//Φ RemoveThread( sv name )ι->sp<Threading::InterruptibleThread>;

		Φ Kill( uint processId )ι->bool;
		Φ StartTime()ι->TimePoint;



		//Φ AddActiveWorker( Threading::IPollWorker* pWorker )ι->void;
		//Φ RemoveActiveWorker( Threading::IPollWorker* p )ι->void;


	protected:




	private:

	};

	struct OSApp final: IApplication{


		Φ SetProductName( sv productName )ι->void;

		α Install( str serviceDescription )ε->void override;
		α Uninstall()ε->void override;


		Φ GetThreadId()ι->uint;
		Φ GetThreadDescription()ι->const char*;

	protected:
		α KillInstance( uint processId )ι->bool override;
		α SetConsoleTitle( sv title )ι->void override;
		α AddSignals()ε->void override;
		α AsService()ι->bool override;

	private:
#ifdef _MSC_VER
		α HandlerRoutine( DWORD  ctrlType )->BOOL;
#endif
	};

	Ŧ Process::AddPollster(  )ι->sp<T>{
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
	*/
}
#undef Φ
#undef Φ
#endif