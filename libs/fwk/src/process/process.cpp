#include <jde/fwk/process/process.h>
#include <iostream>
#include <typeinfo>
#include <sys/types.h>

#include <jde/fwk/settings.h>
#include <jde/fwk/io/Cache.h>
#include <jde/fwk/io/file.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/process/thread.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/Vector.h>

#define let const auto

namespace Jde{
	constexpr ELogTags _tags = ELogTags::App;
	string _applicationName;
	α Process::AppName()ι->const string&{ return _applicationName; }

	bool _isConsole{};
	α Process::SetConsole( bool value )ι->void{ _isConsole=value; }
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
			std::ostringstream args;
			for( auto i=0; i<argc; ++i )
				args << argv[i] << " ";
			if( FindArg("-tests") || FindArg("-ctest") ){
				std::cout
#if _WIN32
					<< "export REPO_BUILD_DIR=\"" << GetEnv("REPO_BUILD_DIR").value_or("") << "\"" << std::endl
     			<< "export REPO_SOURCE_DIR=\"" << GetEnv("REPO_SOURCE_DIR").value_or("") << "\"" << std::endl
#else
					<< "REPO_BUILD_DIR=" << GetEnv("REPO_BUILD_DIR").value_or("") << std::endl
					<< "REPO_SOURCE_DIR=" << GetEnv("REPO_SOURCE_DIR").value_or("") << std::endl
#endif
					<< "@" << fs::current_path().string() << std::endl
					<< args.str() << std::endl;
			}
			INFO( "Program: {}", args.str() );
			INFO( "pid/cwd: ({}){}", ProcessId(), fs::current_path().string() );
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
		Thread::SetName( appName );
		Process::AddSignals();
		Cache::Init();
		return values;
	}

	optional<int> _exitReason;
	bool _terminate{};
	α Process::ExitReason()ι->optional<int>{ return _exitReason; }
	α Process::SetExitReason( int reason, bool terminate )ι->void{ _exitReason = reason; _terminate = terminate; }
	α Process::ShuttingDown()ι->bool{ return (bool)_exitReason; }
	bool _finalizing{};
	α Process::Finalizing()ι->bool{ return _finalizing; }

	α Process::ExitException( std::exception&& e )ι->int{
		int y{ EXIT_FAILURE };
		string message;
		if( auto p = dynamic_cast<Exception*>(&e); p ){
			y = p->Level()==ELogLevel::Trace ? EXIT_SUCCESS :
				p->Code() ? ( int )p->Code() : EXIT_FAILURE;
			message = p->What();//What(), not what() - it forces the lazy format/args expansion.
			if( message.empty() ){//no format, no args, no inner - all that is left is where it came from.
				let& sl = p->Source();
				message = Ƒ( "[{}] no message, thrown at {}({}) '{}', code={:x}", typeid(*p).name(), sl.file_name(), sl.line(), sl.function_name(), p->Code() );
			}
		}
		else if( let what = e.what(); what && *what )
			message = what;
		if( message.empty() )//e.g. a default-constructed derived std::exception whose what() is ""
			message = Ƒ( "[{}] no message", typeid(e).name() );

		sv prefix = y==0 ? "Exiting on exception:  " : "Exiting on error:  ";
		std::cerr << prefix << message << std::endl;
		return y;
	}

	vector<function<void( bool, SL )>> _shutdownFunctions;
	α Process::AddShutdownFunction( function<void(bool, SL)>&& shutdown )ι->void{
		_shutdownFunctions.push_back( shutdown );
	}

	up<IShutdown> _executor;
	α Process::SetExecutor( up<IShutdown>&& executor )ι->void{
		_executor = move( executor );
	}


	Vector<IShutdown*> _rawShutdowns;
	α Process::AddShutdown( IShutdown* shutdown )ι->void{
		ASSERT( !_rawShutdowns.find(shutdown) );
		_rawShutdowns.push_back( shutdown );
	}
	//No assert on absence: Shutdown() below drains the whole container before invoking anything, so by the time the
	//registrants are destroyed - which is where they unregister - there is nothing left to find.  That is the ordinary
	//path, not a bug.
	α Process::RemoveShutdown( IShutdown* shutdown )ι->void{
		_rawShutdowns.erase( shutdown );
	}


	Ω cleanup( bool terminate )ι->void;
	α Process::Shutdown( int exitReason )ι->void{
		static std::atomic_flag _ran;
		if( _ran.test_and_set() )//linux signal exits run this twice - Pause() calls it before returning, then main calls it with Pause's result; the 2nd pass re-ran the shutdown functions against already-nulled state (e.g. AppServer's AccessListener).
			return;
		if( !ExitReason() )//ExitHandler may have recorded the reason & terminate flag (SIGTERM) already - first cause wins.
			SetExitReason( exitReason, false );
		let terminate = _terminate;

		for_each( _shutdownFunctions, [=](let& shutdown){shutdown(terminate, SRCE_CUR);} );
		DBGT( ELogTags::App | ELogTags::Shutdown, "{} Shutdown functions removed", _shutdownFunctions.size() );
		_shutdownFunctions.clear();
		_rawShutdowns.rerase( [=](auto& p){
			p->Shutdown( terminate );
		});
		DBGT( ELogTags::App | ELogTags::Shutdown, "Raw functions removed" );
		cleanup( terminate );
	}

	Ω cleanup( bool terminate )ι->void{
		INFOT( ELogTags::App, "Clearing Logger" );
		std::this_thread::sleep_for( 100ms );
		_finalizing = true;
		Cache::Shutdown( terminate );
		auto ioc = ExecutorIoc();//keep the io_context alive across teardown so it is destroyed last — after sessions, loggers and timers release their asio objects.
		if( _executor ){
			_executor->Shutdown( terminate );
			_executor = nullptr;
		}
		Logging::DestroyLoggers( terminate );
		if( ioc && ioc.use_count()>1 )//everything that used the io_context should have released it by now; a leftover ref means an asio object would otherwise outlive the io_context (use-after-free).
			std::cout << "WARNING: io_context still has " << ioc.use_count()-1 << " reference(s) at finalize." << std::endl;
		ioc = nullptr;//io_context destroyed here, deterministically last.
		std::cout << "Shutdown complete." << std::endl;
	}
	α Process::AppDataFolder()ι->fs::path{
		return ProgramDataFolder()/CompanyRootDir()/Process::ProductName();
	}
	α Process::GetEnv( str variable, bool emptyIsNullOpt )ι->optional<string>{
		optional<string> y;
#ifdef _WIN32
		char* env = nullptr;
		size_t size = 0;
		if( _dupenv_s(&env, &size, variable.c_str()) == 0 && env ){
			y = string{ env };
			free( env );
		}
#else
		if( let env = std::getenv(variable.c_str()); env )
			y = string{ env };
#endif
		return emptyIsNullOpt && y && y->empty() ? optional<string>{} : y;
	}
}