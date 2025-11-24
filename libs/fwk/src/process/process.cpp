#include <jde/fwk/process/process.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include <jde/fwk/settings.h>
#include <jde/fwk/io/Cache.h>
#include <jde/fwk/io/file.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/thread.h>
#include <jde/fwk/utils/Vector.h>

#define let const auto

namespace Jde{
	constexpr ELogTags _tags = ELogTags::App;
	string _applicationName;
	α Process::AppName()ι->const string&{ return _applicationName; }

	bool _isConsole{};
	α Process::SetConsole( bool value )ι->void{ _isConsole=value;}
	α Process::IsConsole()ι->bool{ return _isConsole; }


	TimePoint _startTime{ Clock::now() };
	α Process::StartTime()ι->TimePoint{ return _startTime; };

	function<void()> OnExit;
}
namespace Jde{
	α Process::FindArg( string key )ι->optional<string>{
		auto p = Args().find( key );
		return p!=Args().end() ? p->second : optional<string>{};
	}
}
namespace Jde{
#undef SetConsoleTitle
	α Process::Startup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console )ε->flat_set<string>{
		auto isConsole = console ? *console : Process::FindArg( "-c" ).has_value();
		Process::SetConsole( isConsole );
		IO::Init();
		{
			std::ostringstream os;
			os << "(" << ProcessId() << ")";
			for( auto i=0; i<argc; ++i )
				os << argv[i] << " ";
			os << ";cwd=" << fs::current_path().string();
			INFOT( ELogTags::App, "Starting {}{}", appName, os.str() );
		}
		_applicationName = appName;
		const string arg0{ argv[0] };
		bool terminate = !_debug;
		flat_set<string> values;
		for( int i=1; i<argc; ++i ){
			if( string(argv[i])=="-c" && !console )
				isConsole = true;
			else if( string(argv[i])=="-t" )
				terminate = !terminate;
			else if( string(argv[i])=="-install" ){
				Install( serviceDescription );
				throw Exception{ "successfully installed.", ELogLevel::Trace };
			}
			else if( string(argv[i])=="-uninstall" ){
				Uninstall();
				throw Exception{ "successfully uninstalled.", ELogLevel::Trace };
			}
			else
				values.emplace( argv[i] );
		}
		if( terminate )
			std::set_terminate( OnTerminate );
		if( isConsole )
			Process::SetConsoleTitle( appName );
		else
			AsService();
		SetThreadDscrptn( appName );
		Process::AddSignals();
		return values;
	}

	optional<int> _exitReason;
	bool _terminate{};
	α Process::ExitReason()ι->optional<int>{ return _exitReason; }
	α Process::SetExitReason( int reason, bool terminate )ι->void{ _exitReason = reason; _terminate = terminate; }
	α Process::ShuttingDown()ι->bool{ return (bool)_exitReason; }
	bool _finalizing{};
	α Process::Finalizing()ι->bool{ return _finalizing; }

	α Process::ExitException( exception&& e )ι->int{
		int y{ EXIT_FAILURE };
		auto cerrOutput = [&](){
			sv prefix = y==0 ? "Exiting on exception:  " : "Exiting on error:  ";
			std::cerr << prefix << e.what() << std::endl;
		};
		if( auto p = dynamic_cast<IException*>(&e); p ){
			y = p->Level()==ELogLevel::Trace ? EXIT_SUCCESS :
				p->Code ? (int)p->Code : EXIT_FAILURE;
		}
		cerrOutput();
		return y;
	}

	vector<function<void(bool)>> _shutdownFunctions;
	α Process::AddShutdownFunction( function<void(bool)>&& shutdown )ι->void{
		_shutdownFunctions.push_back( shutdown );
	}

	 Vector<IShutdown*> _rawShutdowns;
	 α Process::AddShutdown( IShutdown* shutdown )ι->void{
	 	ASSERT( !_rawShutdowns.find(shutdown) );
	 	_rawShutdowns.push_back( shutdown );
	 }
	 α Process::RemoveShutdown( IShutdown* pShutdown )ι->void{
	 	ASSERT( _rawShutdowns.find(pShutdown) );
	 	_rawShutdowns.erase( pShutdown );
	 }


	Ω cleanup( bool terminate )ι->void;
	α Process::Shutdown( int exitReason )ι->void{
		bool terminate{ false }; //use case might be if non-terminate took too long
		SetExitReason( exitReason, terminate );//Sets ShuttingDown should be called in OnExit handler

		for_each( _shutdownFunctions, [=](let& shutdown){ shutdown( terminate ); } );
		DBGT( ELogTags::App | ELogTags::Shutdown, "{} Shutdown functions removed", _shutdownFunctions.size() );
		_rawShutdowns.erase( [=](auto& p){ p->Shutdown( terminate );} );
		DBGT( ELogTags::App | ELogTags::Shutdown, "Raw functions removed" );
		cleanup( terminate );
	}

	Ω cleanup( bool terminate )ι->void{
		INFOT( ELogTags::App, "Clearing Logger" );
		_finalizing = true;
		Logging::DestroyLoggers( terminate );
		std::cout << "Shutdown complete." << std::endl;
	}
	α Process::AppDataFolder()ι->fs::path{
		return ProgramDataFolder()/CompanyRootDir()/Process::ProductName();
	}
	α Process::GetEnv( str variable )ι->optional<string>{
		char* env = std::getenv( variable.c_str() );
		return env ? string{env} : optional<string>{};
	}
}