#pragma once
#include "../../Framework/source/threading/Mutex.h"

#define var const auto
namespace Jde::DB::Odbc
{
	struct IWorker /*abstract*/
	{
		virtual bool Poll()ι=0;
		Ŧ static ThreadCount()ι->uint8;
	};

	struct OdbcWorker final: IWorker
	{
		bool Poll()ι override;
		Ω Push( std::coroutine_handle<>&& h, HANDLE hEvent, bool closeEvent=false )ι->void;
		static uint8 _threadCount;
		static constexpr sv Name="Odbc"sv;
	protected:

	private:
	};

	struct WorkerManager
	{
		Ŧ static Start( sv workerName )ι->sp<T>;
	private:
		static vector<up<IWorker>> _workers;  static std::atomic_flag _objectLock;
		static flat_set<sv> _workerNames; static std::atomic_flag _nameLock;
	};

	Ŧ static IWorker::ThreadCount()ι->uint8
	{
		if( T::_threadCount==std::numeric_limits<uint8>::max() )
		{
			T::_threadCount = Settings::FindNumber<uint8>( Jde::format("workders/{}/threads", T::Name) ).value_or( 0 );
		}
		return T::_threadCount;
	}

	Ŧ WorkerManager::Start( sv workerName )ι->sp<T>{
		sp<T> p;
		AtomicGuard l{ _nameLock };
		if( _workerNames.emplace( workerName ).second ){
			l.unlock();
			AtomicGuard l2{ _objectLock };
			//TODO:  load settings, if thread count==0 then work with it here, else create worker.
			var pSettings = Settings::FindObject( "workers", workerName );
			var threads = pSettings ? pSettings->FindNumber<uint8>( "threads" ).value_or(0) : 0;
		}
		return p;
	}
}
#undef var