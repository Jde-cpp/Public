#pragma once
#include <boost/asio/awaitable.hpp>
#include <jde/fwk/process/execution.h>
#include <jde/db/awaits/QueryAwait.h>
#include "MySqlDataSource.h"

namespace Jde::DB::MySql{
	struct MySqlQueryAwait final : TAwait<Result>{
		MySqlQueryAwait( sp<MySqlDataSource> ds, Sql&& s, bool outParams, SL sl )ι;
	private:
		α Suspend()ι->void override;
		α Main()ι->asio::awaitable<void>;
	private:
		sp<boost::asio::io_context> _ctx;
		sp<MySqlDataSource> _ds;
		bool _outParams;
		Sql _sql;
	};
}