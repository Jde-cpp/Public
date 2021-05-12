#pragma once
#include <jde/Exports.h>
#include "Assert.h"
#include "Log.h"
namespace Jde::Threading{ struct InterruptibleThread; }

namespace Jde
{
	namespace IO{ struct IDrive; }
	struct IShutdown
	{
		virtual void Shutdown()noexcept=0;
	};

	struct JDE_NATIVE_VISIBILITY IApplication
	{
		virtual ~IApplication();
		static IApplication& Instance()noexcept{ /*assert(_pInstance);*/ return *_pInstance; }
		set<string> BaseStartup( int argc, char** argv, sv appName, sv companyName="jde-cpp" )noexcept(false);

		static size_t MemorySize()noexcept;
		static fs::path Path()noexcept;
		static string HostName()noexcept;
		static uint ProcessId()noexcept;

		static void AddThread( sp<Threading::InterruptibleThread> pThread )noexcept;
		static void RemoveThread( sp<Threading::InterruptibleThread> pThread )noexcept;
		static void GarbageCollect()noexcept;
		static void AddApplicationLog( ELogLevel level, str value )noexcept;//static to call in std::terminate.
		static void AddShutdown( sp<IShutdown> pShared )noexcept;
		static void Add( sp<void> pShared )noexcept;
		static bool Kill( uint processId )noexcept{return _pInstance ? _pInstance->KillInstance( processId ) : false;}
		static void Remove( sp<void> pShared )noexcept;
		static void CleanUp()noexcept;
		static TimePoint StartTime()noexcept;
		static void AddShutdownFunction( std::function<void()>&& shutdown )noexcept;
		static void Pause()noexcept;
		static vector<sp<Threading::InterruptibleThread>>& GetBackgroundThreads()noexcept{ return  *_pBackgroundThreads; }
		static sv CompanyName()noexcept{ return _pCompanyName ? *_pCompanyName : ""sv;}
		static sv ApplicationName()noexcept{ return _pApplicationName ? *_pApplicationName : ""sv;}
		virtual fs::path ProgramDataFolder()noexcept=0;
		static fs::path ApplicationDataFolder()noexcept;
		static bool ShuttingDown()noexcept{ return _shuttingDown; }
		static sp<IO::IDrive> DriveApi()noexcept;
		void Wait()noexcept;
		virtual string GetEnvironmentVariable( sv variable )noexcept=0;
	protected:

		static void OnTerminate()noexcept;//implement in OSApp.cpp.
		virtual void OSPause()noexcept=0;
		virtual bool AsService()noexcept=0;
		virtual void AddSignals()noexcept(false)=0;
		virtual bool KillInstance( uint processId )noexcept=0;

		static mutex _threadMutex;
		static VectorPtr<sp<Threading::InterruptibleThread>> _pBackgroundThreads;

		static sp<IApplication> _pInstance;
		static unique_ptr<string> _pApplicationName;
		static unique_ptr<string> _pCompanyName;
	private:
		virtual void SetConsoleTitle( sv title )noexcept=0;
		static bool _shuttingDown;
	};

	struct OSApp : IApplication
	{
		JDE_NATIVE_VISIBILITY static set<string> Startup( int argc, char** argv, sv appName )noexcept(false);
		string GetEnvironmentVariable( sv variable )noexcept override;
		fs::path ProgramDataFolder()noexcept override;

		JDE_NATIVE_VISIBILITY uint GetThreadId()noexcept;
		JDE_NATIVE_VISIBILITY void SetThreadDscrptn( std::thread& thread, sv pszDescription )noexcept;
		JDE_NATIVE_VISIBILITY void SetThreadDscrptn( const std::string& pszDescription )noexcept;
		JDE_NATIVE_VISIBILITY const char* GetThreadDescription()noexcept;

	protected:
		bool KillInstance( uint processId )noexcept override;
		void SetConsoleTitle( sv title )noexcept override;
		void AddSignals()noexcept(false) override;
		bool AsService()noexcept override;
		void OSPause()noexcept override;

		//void OnTerminate()noexcept override;
	private:
		static void ExitHandler( int s );
#ifdef _MSC_VER
		BOOL HandlerRoutine( DWORD  ctrlType );
#endif
	};
}