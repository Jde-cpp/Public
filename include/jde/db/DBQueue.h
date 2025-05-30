#pragma once
#include <atomic>
#include <jde/db/exports.h>
#include <jde/db/Value.h>
#include <jde/framework/process.h>
#include "../../../../Framework/source/collections/Queue.h"


namespace Jde::Threading{struct InterruptibleThread;}
namespace Jde::DB
{
	struct QStatement final{
		QStatement( string sql, sp<vector<Value>> parameters, bool isStoredProc, SL sl );//TODO remove this.

		string Sql;//const?
		const sp<vector<Value>> Parameters;
		bool IsStoredProc;//const?
		source_location SourceLocation;
	};

	struct IDataSource;

	struct ΓDB DBQueue final : IShutdown{//, std::enable_shared_from_this<DBQueue>
		DBQueue( sp<IDataSource> spDataSource )ι;
		α Push( string sql, sp<vector<Value>> parameters, bool isStoredProc=true, SRCE )ι->void;
		α Shutdown( bool terminate )ι->void override;
	private:
		α Run()ι->void;
		sp<Threading::InterruptibleThread> _pThread;
		Queue<QStatement> _queue;
		sp<IDataSource> _spDataSource;
		std::atomic<bool> _stopped{false};
	};
}