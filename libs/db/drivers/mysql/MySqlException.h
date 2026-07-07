#pragma once
#include "usings.h"
#include <jde/fwk/exceptions/ExternalException.h>

namespace Jde::DB::MySql{
	struct MySqlException final : ExternalException {
		MySqlException( sv sql, const mysql::error_with_diagnostics& e, SRCE, ELogTags tags=ELogTags::Sql, ELogLevel l=ELogLevel::Error ):
			ExternalException{ string{e.what()}, Ƒ("{} - {}", e.get_diagnostics().server_message(), sql), {l, tags, (uint32)e.code().value()}, sl }
		{}

		α Move()ι->up<Exception> override{ return mu<MySqlException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	};
}