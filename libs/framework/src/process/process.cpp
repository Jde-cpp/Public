#include <jde/framework/process/process.h>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>

#include <jde/framework/settings.h>
#include <jde/framework/io/Cache.h>
#include <jde/framework/io/file.h>
#include <jde/framework/io/FileAwait.h>
#include <jde/framework/process/thread.h>
#include <jde/framework/utils/Vector.h>

#define let const auto

namespace Jde{
	constexpr ELogTags _tags = ELogTags::App;
	string _applicationName;
	α Process::ApplicationName()ι->const string&{ return _applicationName; }

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
			SetConsoleTitle( appName );
		else
			AsService();
		Logging::Initialize();
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

	// α Process::Pause()ι->int{
	// 	INFOT( ELogTags::App, "Pausing main thread." );
	// 	while( !_exitReason ){
			// AtomicGuard l{ _activeWorkersMutex };
			// uint size = _activeWorkers.size();
			// if( size ){
				// l.unlock();
				// bool processed = false;
				// for( uint i=0; i<size; ++i ){
				// 	AtomicGuard l2{ _activeWorkersMutex };
				// 	auto p = i<_activeWorkers.size() ? _activeWorkers[i] : nullptr; if( !p ) break;
				// 	l2.unlock();
				// 	if( let pWorkerProcessed = p->Poll();  pWorkerProcessed )
				// 		processed = *pWorkerProcessed || processed;
				// 	else
				// 		IApplication::RemoveActiveWorker( p );
				// }
				// if( !processed )
				// 	std::this_thread::yield();
//			}
//			else{
//				l.unlock();
//				Process::Pause();
//			}
//		}
	// 	INFOT( "Pause returned = {}.", _exitReason ? std::to_string(_exitReason.value()) : "null" );
	// 	//_backgroundThreads.visit( [](auto&& p){ p->Interrupt(); } );
	// 	Shutdown( _exitReason.value_or(-1) );
	// 	return _exitReason.value_or( -1 );
	// }

	Ω cleanup( bool terminate )ι->void;
	α Process::Shutdown( int exitReason )ι->void{
		bool terminate{ false }; //use case might be if non-terminate took too long
		SetExitReason( exitReason, terminate );//Sets ShuttingDown should be called in OnExit handler
		//_shutdowns.erase( [terminate](auto& p){
		//	p->Shutdown( terminate );
		//});
		INFOT( ELogTags::App | ELogTags::Shutdown, "[{}]Waiting for process to complete. exitReason: {}, terminate: {}", ProcessId(), _exitReason.value(), terminate );
		// while( _backgroundThreads.size() ){
		// 	_backgroundThreads.erase_if( [](let& p)->bool {
		// 		auto done = p->IsDone();
		// 		if( done )
		// 			p->Join();
		// 		else if( done = p->Id()==std::this_thread::get_id(); done )
		// 			p->Detach();
		// 		return done;
		// 	});
		// 	std::this_thread::yield(); //std::this_thread::sleep_for( 2s );
		// }
		DBGT( ELogTags::App | ELogTags::Shutdown, "Background threads removed" );

		for_each( _shutdownFunctions, [=](let& shutdown){ shutdown( terminate ); } );
		DBGT( ELogTags::App | ELogTags::Shutdown, "{} Shutdown functions removed", _shutdownFunctions.size() );
		_rawShutdowns.erase( [=](auto& p){ p->Shutdown( terminate );} );
		DBGT( ELogTags::App | ELogTags::Shutdown, "Raw functions removed" );
		cleanup( terminate );
		_finalizing = true;
	}

	Ω cleanup( bool terminate )ι->void{
		INFOT( ELogTags::App, "Clearing Logger" );
		Logging::DestroyLoggers( terminate );
	}
	α Process::ApplicationDataFolder()ι->fs::path{
		return ProgramDataFolder()/CompanyRootDir()/Process::ProductName();
	}
}