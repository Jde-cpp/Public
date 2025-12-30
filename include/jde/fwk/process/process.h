#pragma once
#ifndef JDE_APP_H
#define JDE_APP_H

#define Φ Γ α
namespace Jde{
	struct IShutdown{
		β Shutdown( bool terminate )ι->void=0;
	};
namespace Process{
	Φ AppName()ι->const string&;
	Φ AppDataFolder()ι->fs::path;
	Φ Args()ι->const flat_multimap<string,string>&;
	Φ FindArg( string key )ι->optional<string>;
	Φ CompanyName()ι->string;
	Φ CompanyRootDir()ι->fs::path;
	Φ GetEnv( str variable )ι->optional<string>;
	Φ Executable()ι->fs::path;
	Φ ExePath()ι->fs::path;
	Φ HostName()ι->string;
	Φ IsConsole()ι->bool;
	Φ IsDebuggerPresent()ι->bool;
	Φ SetConsole( bool isConsole )ι->void;
	Φ SetConsoleTitle( sv title )ι->void;
	Φ MemorySize()ι->size_t;
	Φ ProcessId()ι->uint32;
	constexpr static sv ProductVersion="2025.12.01";
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

	Φ AddShutdownFunction( function<void(bool terminating)>&& shutdown )ι->void;

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

	Φ Install( str serviceDescription )ε->void;
	Φ Uninstall()ε->void;
}}
#undef Φ
#endif