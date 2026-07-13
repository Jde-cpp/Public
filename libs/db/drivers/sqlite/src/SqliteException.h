#pragma once
#include "jde/fwk/usings.h"
#include <jde/fwk/exceptions/ExternalException.h>

namespace Jde::DB::Sqlite{
	struct SqliteException final : public ExternalException{
		template<class... Args>
		SqliteException( SL sl, int rc, fmt::format_string<Args...> m, Args&&... sargs )ι:
			ExternalException{ {sqlite3_errstr(rc)}, Ƒ(FWD(m), FWD(sargs)...), {ELogLevel::Error, ELogTags::Sql, (uint32)rc}, sl }
		{}
	};

}