#include <jde/framework/process.h>
#include <Psapi.h>
#include <shellapi.h>
#include <strsafe.h>
#include "WindowsDrive.h"
#include "WindowsSvc.h"
#include "WindowsWorker.h"
#include "WindowsUtilities.h"

#define var const auto

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::App };

	α OSApp::Startup( int argc, char** argv, sv appName, string serviceDescription, optional<bool> console )ε->flat_set<string>{
		IApplication::SetInstance( ms<OSApp>() );
		return IApplication::Instance().BaseStartup( argc, argv, appName, serviceDescription, console );
	}

	bool OSApp::KillInstance( uint processId )ι{
		Information{ ELogTags::App | ELogTags::Shutdown, "Kill received - stopping instance" };
		var proc = ::OpenProcess( PROCESS_TERMINATE, false, (DWORD)processId );
		if( proc ){
			::TerminateProcess( proc, 1 );
			::CloseHandle( proc );
		}
		return proc;
	}
	void OSApp::SetConsoleTitle( sv title )ι
	{
		::SetConsoleTitle( Jde::format("{}({})", title, ProcessId()).c_str() );
	}

	BOOL HandlerRoutine( DWORD ctrlType ){
		bool handled{ true };
		var tags = ELogTags::App | ELogTags::Shutdown;
		switch( ctrlType ){
			case CTRL_C_EVENT: Information{ tags, "Ctrl-C event" }; break;
			case CTRL_CLOSE_EVENT: Information{ tags, "Ctrl-Close event" }; break;
			case CTRL_BREAK_EVENT: Information{ tags, "Ctrl-Break event" }; break;
			case CTRL_LOGOFF_EVENT: Information{ tags, "Ctrl-Logoff event" }; break;
			case CTRL_SHUTDOWN_EVENT: Information{ tags, "Ctrl-Shutdown event" }; break;
			default: Information{ tags, "Ctrl-C event unhanded: {:x}", ctrlType }; handled = false;
    }
		if( handled )
			Windows::WindowsWorkerMain::Stop( ctrlType );
		return handled;
	}
	void AddSignals2()ε
	{
		if( !SetConsoleCtrlHandler(HandlerRoutine, TRUE) )
			THROW( "Could not set control handler" );
	}
	void OSApp::AddSignals()ε
	{
		AddSignals2();
	}

	size_t IApplication::MemorySize()ι
	{
		PROCESS_MEMORY_COUNTERS memCounter;
		/*BOOL result =*/ ::GetProcessMemoryInfo( ::GetCurrentProcess(), &memCounter, sizeof(memCounter) );
		return memCounter.WorkingSetSize;
	}
	fs::path IApplication::ExePath()ι
	{
		return fs::path( _pgmptr );
	}
	string IApplication::HostName()ι{
		DWORD maxHostName = 1024;
		char hostname[1024];
		if( !::GetComputerNameA(hostname, &maxHostName) )
			strcpy( hostname, "GetComputerNameA failed" );

		return hostname;
	}
	uint32 OSApp::ProcessId()ι{
		return _getpid();
	}

	void IApplication::OnTerminate()ι{
		//TODO Implement
	}

	bool _isService{false};
	α OSApp::AsService()ι->bool{
		_isService = true;
		Windows::Service::ReportStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );
		return true;
	}

	up<flat_multimap<string,string>> _args;
	α Process::Args()ι->const flat_multimap<string,string>&{
		if( !_args ){
			_args = mu<flat_multimap<string,string>>();
			int nArgs;
			LPWSTR* szArglist = ::CommandLineToArgvW( ::GetCommandLineW(), &nArgs );
			if( !szArglist )
				std::cerr << "CommandLineToArgvW failed\n";
		   else{
				optional<string> key;
				for( int i=0; i<nArgs; ++i ){
					var current = Windows::ToString( szArglist[i] );
					if( current.starts_with('-') ){
						if( key )
							_args->emplace( *key, string{} );
						if( uint i=current.find('='); i<current.size() ){
							_args->emplace( current.substr(0, i), current.substr(i+1) );
							key.reset();
						}else
							key = current;
					}
					else if( key )
						_args->emplace( *key, current );
					else
						_args->emplace( string{}, current );
				}
				if( key )
					_args->emplace( *key, string{} );
			  LocalFree(szArglist);
			}
		}
		return *_args;
	}
	α Process::IsDebuggerPresent()ι->bool{
		return ::IsDebuggerPresent() != 0;
	}

	α OSApp::Executable()ι->fs::path{
		return fs::path{ Process::Args().find( {} )->second };
	}

	α OSApp::UnPause()ι->void{
		Windows::WindowsWorkerMain::Stop( 0 );
	}

	α OSApp::Pause()ι->void{
		Information{ ELogTags::App | ELogTags::Startup, "Starting main thread loop...{}", _getpid() };
		if( _isService ){
			SERVICE_TABLE_ENTRY DispatchTable[] = {  { (char*)Process::ApplicationName().data(), (LPSERVICE_MAIN_FUNCTION)Windows::Service::Main },  { nullptr, nullptr }  };
			var success = StartServiceCtrlDispatcher( DispatchTable );//blocks?
			if( !success )
				Windows::Service::ReportEvent( "StartServiceCtrlDispatcher" );
		}
		else
			Windows::WindowsWorkerMain::Start( false );
	}
	string _companyName;

//could get run before initialize logger.
#define CHECK_NOLOG(condition) if( !(condition) ) throw Jde::Exception{ SRCE_CUR, Jde::ELogLevel::NoLog, "error: {}", #condition }
	α LoadResource( sv key )ι->string{
		string y;
		try{
			DWORD _;
			var exe = OSApp::Executable().string();
			var size = ::GetFileVersionInfoSize( exe.c_str(), &_ );
			if( !size )
				return y;
			vector<BYTE> block( size );
			CHECK_NOLOG( ::GetFileVersionInfo(exe.c_str(), _, size, block.data()) );
			struct LANGANDCODEPAGE { WORD wLanguage; WORD wCodePage; } *lpTranslate; UINT cbTranslate;
			::VerQueryValue( block.data(), TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &cbTranslate ); CHECK_NOLOG( (cbTranslate/sizeof(struct LANGANDCODEPAGE)) );
			char name[50];
			CHECK_NOLOG( SUCCEEDED(::StringCchPrintf(name, sizeof(name), Jde::format("\\StringFileInfo\\%04x%04x\\{}", key).c_str(),  lpTranslate[0].wLanguage, lpTranslate[0].wCodePage)) );
			char* pCompanyName; UINT bytes;
			::VerQueryValue( block.data(),  name, (LPVOID*)&pCompanyName, &bytes );
			y = sv{ pCompanyName, bytes-1 };
		}
		catch( IException& )
		{}
		return y;
	}

	α OSApp::CompanyName()ι->string{
		if(! _companyName.size() ){
			_companyName = LoadResource( "CompanyName" );
			if( _companyName.empty() )
				_companyName = "Jde-cpp";
		}
		return _companyName;
	}
	string _productName;
	α OSApp::ProductName()ι->sv{
		if( _productName.empty() ){
			_productName = LoadResource( "ProductName" );
			if( _productName.empty() )
				_productName = "Jde-cpp";
		}
		return _productName;
	}
	α OSApp::SetProductName( sv productName )ι->void{
		if( ProductName() == "Jde-cpp" )
			_productName = productName;
	}
	α OSApp::CompanyRootDir()ι->fs::path{ return CompanyName(); }

	α IApplication::EnvironmentVariable( str variable, SL sl )ι->optional<string>{
		char buffer[32767];
		optional<string> result;
		if( !::GetEnvironmentVariable(string(variable).c_str(), buffer, sizeof(buffer)) )
			Logging::LogOnce( Logging::Message{ELogLevel::Debug, "GetEnvironmentVariable('{}') failed:  {:x}", sl}, ELogTags::Settings, variable, ::GetLastError() );
		else
			result = buffer;

		return result;
	}
	fs::path IApplication::ProgramDataFolder()ι{
		return fs::path{ EnvironmentVariable("ProgramData").value_or("") };
	}

	struct SCDeleter
	{
		void operator()(SC_HANDLE p){ if( p ) ::CloseServiceHandle(p); }
	};
	using ServiceHandle = std::unique_ptr<SC_HANDLE__, SCDeleter>;

	ServiceHandle MyOpenSCManager()ε{
		auto schSCManager = ServiceHandle{ ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS) };
		if( !schSCManager.get() ){
			if( ::GetLastError() == ERROR_ACCESS_DENIED )
				THROW( "installation requires administrative privliges." );
			else
				THROW( "OpenSCManager failed - {}", ::GetLastError() );
		}
		return schSCManager;
	}

	α OSApp::Install( str serviceDescription )ε->void
	{
		auto schSCManager = MyOpenSCManager();
		//auto schSCManager = ::OpenSCManager( nullptr, nullptr, SC_MANAGER_ALL_ACCESS ); THROW_IF( schSCManager==nullptr, "OpenSCManager failed - {}", ::GetLastError() );
		const string serviceName{ Process::ApplicationName() };
		auto service = ServiceHandle{ ::CreateService(schSCManager.get(), serviceName.c_str(), (serviceName).c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, ExePath().string().c_str(), nullptr, nullptr, nullptr, nullptr, nullptr) };
		if( !service.get() ){
			if( ::GetLastError()==ERROR_SERVICE_EXISTS )
				THROW( "Service allready exists." );
			else
				THROW( "CreateService failed - {}", ::GetLastError() );
		}
		if( serviceDescription.size() ){
			SERVICE_DESCRIPTION d{ (LPSTR)serviceDescription.c_str() };
			if( !::ChangeServiceConfig2A(service.get(), SERVICE_CONFIG_DESCRIPTION, &d) )
				std::cerr << "ChangeServiceConfig2A failed" << std::endl;
		}
		Information{ ELogTags::App, "service '{}' installed successfully", serviceName };
	}
	α OSApp::Uninstall()ε->void{
		auto manager = MyOpenSCManager();
		auto service = ServiceHandle{ ::OpenService(manager.get(), Process::ApplicationName().c_str(), DELETE) };
		if( !service.get() ){
			if( ::GetLastError()==ERROR_SERVICE_DOES_NOT_EXIST )
				THROW( "Service '{}' not found.", Process::ApplicationName() );
			else
				THROW( "DeleteService failed - {}", ::GetLastError() );
		}
		THROW_IF( !::DeleteService(service.get()), "DeleteService failed:  {:x}", GetLastError() );

		Information{ ELogTags::App, "Service '{}' deleted successfully"sv, Process::ApplicationName() };
	}

	α OSApp::LoadLibrary( const fs::path& path )ε->void*{
		auto p = ::LoadLibrary( path.string().c_str() ); THROW_IFX( !p, IOException(path, GetLastError(), "Can not load library") );
		Information{ ELogTags::App, "({})Opened"sv, path.string() };
		return p;
	}

	α OSApp::FreeLibrary( void* p )ι->void{
		::FreeLibrary( (HMODULE)p );
	}
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
	α OSApp::GetProcAddress( void* pModule, str procName )ε->void*{
		auto p = ::GetProcAddress( (HMODULE)pModule, procName.c_str() ); CHECK( p );
		return p;
	}
}