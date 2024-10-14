#include <syslog.h>
#include <execinfo.h>
#include <signal.h>
#include <dlfcn.h>

#include "LinuxDrive.h"
#include <jde/framework/process.h>
#include "../../Framework/source/threading/InterruptibleThread.h"

#define var const auto
namespace Jde{
	auto _tag{ ELogTags::App };
	α OSApp::FreeLibrary( void* p )ι->void{
		::dlclose( p );
	}

	α OSApp::LoadLibrary( const fs::path& path )ε->void*{
		auto p = ::dlopen( path.c_str(), RTLD_LAZY );
		THROW_IFX( !p, IO_EX(path, ELogLevel::Error, "Can not load library - '{}'", dlerror()) );
		Information( _tag, "[{}] Opened"sv, path.string() );
		return p;
	}
	α OSApp::GetProcAddress( void* pModule, str procName )ε->void*{
		auto p = ::dlsym( pModule, procName.c_str() ); CHECK( p );
		return p;
	}
	α OSApp::Install( str serviceDescription )ε->void{
		THROW( "Not Implemeented" );
	}
	α OSApp::UnPause()ι->void{ ASSERT(false); }//not sure of use case
	α OSApp::Uninstall()ε->void{
		THROW( "Not Implemeented");
	}

	α OSApp::Executable()ι->fs::path{
		return fs::path{ program_invocation_name };
	}

	α IApplication::AddApplicationLog( ELogLevel level, str value )ι->void{ //called onterminate, needs to be static.
		auto osLevel = LOG_DEBUG;
		if( level==ELogLevel::Debug )
			osLevel = LOG_INFO;
		else if( level==ELogLevel::Information )
			osLevel = LOG_NOTICE;
		else if( level==ELogLevel::Warning )
			osLevel = LOG_WARNING;
		else if( level==ELogLevel::Error )
			osLevel = LOG_ERR;
		else if( level==ELogLevel::Critical )
			osLevel = LOG_CRIT;
		syslog( osLevel, "%s",  value.c_str() );
	}
	const string _companyName{ "Jde-Cpp" }; string _productName{ "productName" };
	α OSApp::CompanyName()ι->string{ return _companyName; }
	//α OSApp::SetProductName( sv n )ι->str{ _productName=v; }
	α OSApp::ProductName()ι->sv{ return _productName; }
	α OSApp::SetProductName( sv productName )ι->void{ _productName = productName; }
	α IApplication::MemorySize()ι->size_t{//https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
		uint size = 0;
		FILE* fp = fopen( "/proc/self/statm", "r" );
		if( fp!=nullptr ){
			long rss = 0L;
			if( fscanf( fp, "%*s%ld", &rss ) == 1 )
				size = (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
			fclose( fp );
		}
		return size;
	}

	α IApplication::ExePath()ι->fs::path{ return fs::canonical( "/proc/self/exe" ); }

	string IApplication::HostName()ι{
		constexpr uint maxHostName = HOST_NAME_MAX;
		char hostname[maxHostName];
		::gethostname( hostname, maxHostName );
		return hostname;
	}

	uint OSApp::ProcessId()ι{ return getpid(); }

	α OSApp::Startup( int argc, char** argv, sv appName, string serviceDescription )ε->flat_set<string>{
		auto pInstance = ms<OSApp>();
		IApplication::SetInstance( pInstance );
		return pInstance->BaseStartup( argc, argv, appName, serviceDescription );
	}
	atomic<bool> _workerMutex{false};
	vector<sp<Threading::IWorker>> _workers;

	α OSApp::Pause()ι->void{
//		LOG( "Pausing main thread. {}", getpid() );
		::pause();
/*		for( ;; )
		{
			break;//not implemented yet.
			auto pWorker = _activeWorkers.WaitAndPop();
			if( pWorker->Poll() )
				AddActiveWorker( pWorker );//make sure doesn't loop forever.
		}*/
//		LOG( "::pause errno={}.", errno );
		//IApplication::Wait();
	}

	bool OSApp::AsService()ι{
		return ::daemon( 1, 0 )==0;
	}

	α IApplication::OnTerminate()ι->void{
		void *trace_elems[20];
		auto trace_elem_count( backtrace(trace_elems, 20) );
		char **stack_syms( backtrace_symbols(trace_elems, trace_elem_count) );
		std::ostringstream os;
		for( auto i = 0; i < trace_elem_count ; ++i )
			os << stack_syms[i] << std::endl;

		IApplication::AddApplicationLog( ELogLevel::Critical, os.str() );
		free( stack_syms );
		exit( EXIT_FAILURE );
	}

	α IApplication::EnvironmentVariable( str variable, SL sl )ι->optional<string>{
		char* pEnv = std::getenv( string{variable}.c_str() );
		return pEnv ? string{pEnv} : optional<string>{};
	}

	α IApplication::ProgramDataFolder()ι->fs::path{
		return fs::path{ EnvironmentVariable("HOME").value_or("/") };
	}

	α OSApp::ExitHandler( int s )->void{
		if( !Process::ExitReason() )
			Process::SetExitReason( s, s==SIGTERM );
		//Handled in main.cpp
		//ASSERT( false ); //TODO handle
	//	signal( s, SIG_IGN );
	//not supposed to log here...
		//printf( "!!!!!!!!!!!!!!!!!!!!!Caught signal %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",s );
		//pProcessManager->Stop();
		//delete pLogger; pLogger = nullptr;
		//exit( 1 );
	}

	α OSApp::KillInstance( uint processId )ι->bool{
		var result = ::kill( processId, 14 );
		if( result )
			Error{ _tag, "kill failed with '{}'.", result };
		else
			Information{ _tag, "kill sent to:  '{}'.", processId };
		return result==0;
	}

	up<flat_multimap<string,string>> _pArgs;
	α OSApp::Args()ι->const flat_multimap<string,string>&{
		if( !_pArgs ){
			_pArgs = mu<flat_multimap<string,string>>();
			std::ifstream file( "/proc/self/cmdline" );
			optional<string> key;
			for( string current; std::getline<char>(file, current, '\0'); ){
				if( current.starts_with('-') ){
					if( key )
						_pArgs->emplace( *key, string{} );
					key = current;
				}
				else if( key )
					_pArgs->emplace( *key, current );
				else
					_pArgs->emplace( string{}, current );
			}
			if( key )
				_pArgs->emplace( *key, string{} );
		}
		return *_pArgs;
	}
	α OSApp::CompanyRootDir()ι->fs::path{ return fs::path{ "."+OSApp::CompanyName() }; };

	α OSApp::AddSignals()ε->void{/*ε for windows*/
/* 		struct sigaction sigIntHandler;//_XOPEN_SOURCE
		memset( &sigIntHandler, 0, sizeof(sigIntHandler) );
		sigIntHandler.sa_handler = ExitHandler;
		sigemptyset( &sigIntHandler.sa_mask );
		sigIntHandler.sa_flags = 0;*/
		signal( SIGINT, OSApp::ExitHandler );
		signal( SIGSTOP, OSApp::ExitHandler );
		signal( SIGKILL, OSApp::ExitHandler );
		signal( SIGTERM, OSApp::ExitHandler );
		signal( SIGALRM, OSApp::ExitHandler );
		signal( SIGUSR1, OSApp::ExitHandler );

/*		struct sigaction sa;
	   sa.sa_flags = SA_RESTART | SA_SIGINFO;
		sigemptyset( &sa.sa_mask );
		sa.sa_sigaction = IO::LinuxDriveWorker::AioSigHandler;
		THROW_IF( ::sigaction(SIGUSR1, &sa, nullptr)==-1,  "sigaction(SIGUSR1) returned {}", errno );
*/
		//sigaction( SIGSTOP, &sigIntHandler, nullptr );
		//sigaction( SIGKILL, &sigIntHandler, nullptr );
		//sigaction( SIGTERM, &sigIntHandler, nullptr );
	}

	α OSApp::SetConsoleTitle( sv title )ι->void{
		std::cout << "\033]0;" << title << "\007";
	}
}