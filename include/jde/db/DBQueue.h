#pragma once
#include <atomic>
#include <jde/framework/process.h>
#include <jde/db/exports.h>
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>
#include "../../../../Framework/source/collections/Queue.h"

#ifdef Unused
namespace Jde::Threading{struct InterruptibleThread;}
namespace Jde::DB{

	struct QStatement final{
		QStatement( Sql&& sql, SRCE );//TODO remove this.

		DB::Sql _sql;
		SL _sl;
	};

	struct IDataSource;

	struct ΓDB DBQueue final : IShutdown{//, std::enable_shared_from_this<DBQueue>
		DBQueue( sp<IDataSource> spDataSource )ι;
		α Push( Sql&& sql, SRCE )ι->void;
		α Shutdown( bool terminate )ι->void override;
	private:
		α Run()ι->void;
		sp<Threading::InterruptibleThread> _pThread;
		Queue<QStatement> _queue;
		sp<IDataSource> _spDataSource;
		std::atomic<bool> _stopped{false};
	};
}
#endif