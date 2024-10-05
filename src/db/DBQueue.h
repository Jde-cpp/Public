#pragma once
#include <jde/db/DataType.h>
#include <atomic>
#include "../../../Framework/source/collections/Queue.h"
#include <jde/Exports.h>
#include <jde/App.h>

namespace Jde::Threading{struct InterruptibleThread;}
namespace Jde::DB
{
	struct Statement final
	{
		Statement( string sql, sp<vector<object>> parameters, bool isStoredProc, SL sl );//TODO remove this.

		string Sql;//const?
		const sp<vector<object>> Parameters;
		bool IsStoredProc;//const?
		source_location SourceLocation;
	};

	struct IDataSource;

	struct Γ DBQueue final : IShutdown//, std::enable_shared_from_this<DBQueue>
	{
		DBQueue( sp<IDataSource> spDataSource )ι;
		α Push( string sql, sp<vector<object>> parameters, bool isStoredProc=true, SRCE )ι->void;
		α Shutdown( bool terminate )ι->void override;
	private:
		α Run()ι->void;
		sp<Threading::InterruptibleThread> _pThread;
		Queue<Statement> _queue;
		sp<IDataSource> _spDataSource;
		std::atomic<bool> _stopped{false};
	};
}