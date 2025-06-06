#include <jde/db/DBQueue.h>
#include <jde/db/IDataSource.h>
#include "../../../../Framework/source/threading/InterruptibleThread.h"

#define let const auto
namespace Jde::DB{
	QStatement::QStatement( Sql&& sql, SL sl ):
		_sql{ move(sql) },
		_sl{ sl }
	{}

	DBQueue::DBQueue( sp<IDataSource> spDataSource )ι:
		_spDataSource{ spDataSource }{
		_pThread = ms<Threading::InterruptibleThread>( "DBQueue", [&](){Run();} );
		IApplication::AddThread( _pThread );
	}

	void DBQueue::Shutdown( bool /*terminate*/ )ι{
		_pThread->Interrupt();
		while( !_stopped )
			std::this_thread::yield();
		//_queue.Push( sp<QStatement>{} );
	}

	void DBQueue::Push( Sql&& sql, SL sl )ι{
		if( _stopped )
			return;
		try{
			_spDataSource->ExecuteNoLog( move(sql), sl );
		}
		catch( const IException& e ){
			e.SetLevel( ELogLevel::NoLog );
		}
	}

	void DBQueue::Run()ι
	{
	//	Threading::SetThreadDescription( "DBQueue" );
		while( !Threading::GetThreadInterruptFlag().IsSet() || !_queue.Empty() ){
			let statement = _queue.WaitAndPop( 1s );
			if( !statement )
				continue;
			try{
				_spDataSource->ExecuteNoLog( move(statement->_sql), statement->_sl );
			}
			catch( const IException& ){
				//DB::LogNoServer( pStatement->Sql, pStatement->Parameters.get(), ELogLevel::Error, e.what(), pStatement->SourceLocation );
			}
		}
		_stopped = true;
		_spDataSource = nullptr;
		Trace{ ELogTags::App, "DBQueue::Run - Stopped" };
	}
}