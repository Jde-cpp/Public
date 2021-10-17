#pragma once
#include <jde/Exports.h>
#include "Assert.h"
#include "Log.h"
namespace Jde::Threading{ struct InterruptibleThread; struct IWorker; }
#define 🚪 JDE_NATIVE_VISIBILITY auto
namespace Jde
{
	namespace Threading{ struct IPollWorker; }
	struct IShutdown
	{
		virtual void Shutdown()noexcept=0;
	};
	struct IPollster
	{
		virtual void WakeUp()noexcept=0;
		virtual void Sleep()noexcept=0;
	};

	struct JDE_NATIVE_VISIBILITY IApplication //: IPollster
	{
		virtual ~IApplication();
		static IApplication& Instance()noexcept{ /*assert(_pInstance);*/ return *_pInstance; }
		set<string> BaseStartup( int argc, char** argv, sv appName, string serviceDescription/*, sv companyName="jde-cpp"*/ )noexcept(false);
		virtual void Install( str serviceDescription )noexcept(false)=0;
		virtual void Uninstall()noexcept(false)=0;

		static size_t MemorySize()noexcept;
		static fs::path Path()noexcept;
		static string HostName()noexcept;

		Ω AddThread( sp<Threading::InterruptibleThread> pThread )noexcept->void;
		Ω RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept->void;
		Ω RemoveThread( sv name )noexcept->sp<Threading::InterruptibleThread>;
		Ω GarbageCollect()noexcept->void;
		Ω AddApplicationLog( ELogLevel level, str value )noexcept->void;//static to call in std::terminate.
		Ω AddShutdown( sp<IShutdown> pShared )noexcept->void;
		Ω RemoveShutdown( sp<IShutdown> pShared )noexcept->void;
		Ω Add( sp<void> pShared )noexcept->void;
		Ω Exit( int reason )noexcept->void;
		Ω Kill( uint processId )noexcept{return _pInstance ? _pInstance->KillInstance( processId ) : false;}
		Ω Remove( sp<void> pShared )noexcept->void;
		Ω CleanUp()noexcept->void;
		Ω StartTime()noexcept->TimePoint;
		Ω AddShutdownFunction( std::function<void()>&& shutdown )noexcept->void;
		Ω Pause()noexcept->void;
		Ω IsConsole()noexcept->bool;
		//Ω SetExitReason( int i )noexcept->void;
		//void WakeUp()noexcept override;
		//void Sleep()noexcept override;

		static vector<sp<Threading::InterruptibleThread>>& GetBackgroundThreads()noexcept{ return  *_pBackgroundThreads; }
		static sv ApplicationName()noexcept{ return _pApplicationName ? *_pApplicationName : ""sv;}
		virtual fs::path ProgramDataFolder()noexcept=0;
		static fs::path ApplicationDataFolder()noexcept;
		Ω ShuttingDown()noexcept->bool;
		static void Shutdown()noexcept;
		virtual string GetEnvironmentVariable( sv variable )noexcept=0;
		static void AddActiveWorker( Threading::IPollWorker* pWorker )noexcept;
		static void RemoveActiveWorker( Threading::IPollWorker* p )noexcept;
	protected:

		static void OnTerminate()noexcept;//implement in OSApp.cpp.
		//virtual void OSPause()noexcept=0;
		virtual bool AsService()noexcept=0;
		virtual void AddSignals()noexcept(false)=0;
		virtual bool KillInstance( uint processId )noexcept=0;

		static mutex _threadMutex;
		static VectorPtr<sp<Threading::InterruptibleThread>> _pBackgroundThreads;

		static sp<IApplication> _pInstance;
		static unique_ptr<string> _pApplicationName;
		//static unique_ptr<string> _pCompanyName;
	private:
		virtual void SetConsoleTitle( sv title )noexcept=0;
		//static bool _shuttingDown;
	};

	struct OSApp : IApplication
	{
		static 🚪 Startup( int argc, char** argv, sv appName, string serviceDescription )noexcept(false)->set<string>;
		α GetEnvironmentVariable( sv variable )noexcept->string override;
		α ProgramDataFolder()noexcept->fs::path override;
		Ω CompanyName()noexcept->string;
		Ω CompanyRootDir()noexcept->fs::path;
		Ω FreeLibrary( void* p )noexcept->void;
		Ω LoadLibrary( path path )noexcept(false)->void*;
		Ω GetProcAddress( void* pModule, str procName )noexcept(false)->void*;
		α Install( str serviceDescription )noexcept(false)->void override;
		α Uninstall()noexcept(false)->void override;
		static 🚪 ProcessId()noexcept->uint;
		Ω Executable()noexcept->fs::path;
		Ω Args()noexcept->flat_map<string,string>;
		Ω Pause()noexcept->void;
		🚪 GetThreadId()noexcept->uint;
		🚪 SetThreadDscrptn( std::thread& thread, sv pszDescription )noexcept->void;
		🚪 SetThreadDscrptn( const std::string& pszDescription )noexcept->void;
		🚪 GetThreadDescription()noexcept->const char*;

	protected:
		bool KillInstance( uint processId )noexcept override;
		void SetConsoleTitle( sv title )noexcept override;
		void AddSignals()noexcept(false) override;
		bool AsService()noexcept override;

		//void OnTerminate()noexcept override;
	private:
		static void ExitHandler( int s );
#ifdef _MSC_VER
		BOOL HandlerRoutine( DWORD  ctrlType );
#endif
	};
}
#undef 🚪