#include "usings.h"
namespace Jde::DB::MySql{
	struct MySqlException final : public IException {
		MySqlException( ELogTags tags, SL sl, ELogLevel level, mysql::error_with_diagnostics&& e, string&& sql ):
			IException{ e.what(), level, 0, tags, sl },
			_clientMessage{ e.get_diagnostics().client_message() },
			_serverMessage{ e.get_diagnostics().server_message() },
			_sql{ move(sql) }
		{}
		MySqlException( string&& sql, mysql::error_with_diagnostics&& e, SL sl ):
			MySqlException{ ELogTags::Sql, sl, ELogLevel::Error, move(e), move(sql) }
		{}

		α Move()ι->up<IException> override{ return mu<MySqlException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		string _clientMessage;
		string _serverMessage;
		string _sql;
	};
}