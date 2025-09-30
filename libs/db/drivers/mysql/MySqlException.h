#include "usings.h"
#include <jde/fwk/exceptions/ExternalException.h>

namespace Jde::DB::MySql{
	struct MySqlException final : ExternalException {
		MySqlException( string&& sql, const mysql::error_with_diagnostics& e, SRCE, ELogTags tags=ELogTags::Sql, ELogLevel l=ELogLevel::Error ):
			ExternalException{ string{e.what()}, Ƒ("{} - {}", e.get_diagnostics().server_message(), sql), sl, tags, l },
			_clientMessage{ e.get_diagnostics().client_message() },
			_serverMessage{ e.get_diagnostics().server_message() },
			_sql{ move(sql) }
		{}

		α Move()ι->up<IException> override{ return mu<MySqlException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		string _clientMessage;
		string _serverMessage;
		string _sql;
	};
}