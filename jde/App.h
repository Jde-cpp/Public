#pragma once
#include <jde/Exports.h>
#include "Assert.h"
#include "Log.h"
namespace Jde::Threading{ struct InterruptibleThread; struct IWorker; }

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

		static void AddThread( sp<Threading::InterruptibleThread> pThread )noexcept;
		static void RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept;
		static void GarbageCollect()noexcept;
		static void AddApplicationLog( ELogLevel level, str value )noexcept;//static to call in std::terminate.
		static void AddShutdown( sp<IShutdown> pShared )noexcept;
		static void RemoveShutdown( sp<IShutdown> pShared )noexcept;
		static void Add( sp<void> pShared )noexcept;
		static void Exit( int reason )noexcept;
		static bool Kill( uint processId )noexcept{return _pInstance ? _pInstance->KillInstance( processId ) : false;}
		static void Remove( sp<void> pShared )noexcept;
		static void CleanUp()noexcept;
		static TimePoint StartTime()noexcept;
		static void AddShutdownFunction( std::function<void()>&& shutdown )noexcept;
		static void Pause()noexcept;
		Ω IsConsole()noexcept->bool;
		//void WakeUp()noexcept override;
		//void Sleep()noexcept override;

		static vector<sp<Threading::InterruptibleThread>>& GetBackgroundThreads()noexcept{ return  *_pBackgroundThreads; }
		static string CompanyName()noexcept;//{ return _pCompanyName ? *_pCompanyName : ""sv;}
		static sv ApplicationName()noexcept{ return _pApplicationName ? *_pApplicationName : ""sv;}
		virtual fs::path ProgramDataFolder()noexcept=0;
		static fs::path ApplicationDataFolder()noexcept;
		static bool ShuttingDown()noexcept{ return _shuttingDown; }
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
		static bool _shuttingDown;
	};

	struct OSApp : IApplication
	{
		JDE_NATIVE_VISIBILITY static set<string> Startup( int argc, char** argv, sv appName, string serviceDescription )noexcept(false);
		string GetEnvironmentVariable( sv variable )noexcept override;
		fs::path ProgramDataFolder()noexcept override;
		Ω CompanyRootDir()noexcept->fs::path; //#ifdef _MSC_VER sv company = CompanyName();#else
		void Install( str serviceDescription )noexcept(false) override;
		void Uninstall()noexcept(false) override;
		static uint ProcessId()noexcept;
		static fs::path Executable()noexcept;
		Ω Args()noexcept->flat_map<string,string>;
		JDE_NATIVE_VISIBILITY uint GetThreadId()noexcept;
		JDE_NATIVE_VISIBILITY void SetThreadDscrptn( std::thread& thread, sv pszDescription )noexcept;
		JDE_NATIVE_VISIBILITY void SetThreadDscrptn( const std::string& pszDescription )noexcept;
		JDE_NATIVE_VISIBILITY const char* GetThreadDescription()noexcept;

	protected:
		bool KillInstance( uint processId )noexcept override;
		void SetConsoleTitle( sv title )noexcept override;
		void AddSignals()noexcept(false) override;
		bool AsService()noexcept override;
		//void OSPause()noexcept override;

		//void OnTerminate()noexcept override;
	private:
		static void ExitHandler( int s );
#ifdef _MSC_VER
		BOOL HandlerRoutine( DWORD  ctrlType );
#endif
	};

}