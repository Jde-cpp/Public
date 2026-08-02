#pragma once
#include "jde/fwk/log/logTags.h"
#include "jde/fwk/usings.h"
#include "usings.h"
#include <jde/db/DBException.h>

namespace Jde::DB::MySql{
	//error_code::value() is only an errno when the category says so - client_errc reuses small integers in its own
	//category, so an unqualified value() would collide with the server numbers below.
	Ξ ToDbError( const boost::system::error_code& ec )ι->EDbError{
		if( ec.category()==mysql::get_client_category() )
			return EDbError::Connection; //client_errc is all protocol/handshake/deserialization - nothing the server diagnosed.
		if( ec.category()!=mysql::get_common_server_category() && ec.category()!=mysql::get_mysql_server_category() && ec.category()!=mysql::get_mariadb_server_category() )
			return EDbError::None; //asio/ssl/system categories - not a server diagnostic.
		using errc = mysql::common_server_errc;
		switch( (errc)ec.value() ){
		case errc::er_dup_entry: case errc::er_dup_entry_with_key_name: case errc::er_dup_key: case errc::er_dup_unique: return EDbError::Duplicate;
		case errc::er_no_referenced_row: case errc::er_no_referenced_row_2: case errc::er_row_is_referenced: case errc::er_row_is_referenced_2: return EDbError::ForeignKey;
		case errc::er_bad_null_error: return EDbError::NotNull;
		case errc::er_lock_deadlock: return EDbError::Deadlock;
		case errc::er_lock_wait_timeout: return EDbError::Timeout;
		case errc::er_access_denied_error: case errc::er_dbaccess_denied_error: case errc::er_tableaccess_denied_error: return EDbError::Permission;
		case errc::er_parse_error: return EDbError::Syntax;
		case errc::er_signal_exception: case errc::er_signal_warn: case errc::er_signal_not_found: return EDbError::App; //`signal sqlstate '45000'` in a proc - always 1644, whatever the sqlstate.
		default: return EDbError::None;
		}
	}

	struct MySqlException final : DBException{
		MySqlException( DB::Sql&& sql, const mysql::error_with_diagnostics& e, SRCE, ExceptionArgs args={ELogLevel::Error, ELogTags::Sql} ):
			MySqlException{ move(sql), e, Ƒ("{} - {}", e.what(), e.get_diagnostics().server_message()), args, sl }
		{}
		//no statement to attach - connect/handshake failures, where `context` is a connection description, not sql.
		MySqlException( sv context, const mysql::error_with_diagnostics& e, ExceptionArgs args, SRCE ):
			MySqlException{ DB::Sql{}, e, Ƒ("{} - {} - {}", e.what(), e.get_diagnostics().server_message(), context), args, sl }
		{}
		α UserError()Ι->string override{ return _serverMessage.size() ? _serverMessage : DBException::UserError(); }

		α Move()ι->up<Exception> override{ return mu<MySqlException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		MySqlException( DB::Sql&& sql, const mysql::error_with_diagnostics& e, string what, ExceptionArgs args, SL sl ):
			DBException{
				ToDbError(e.code()),
				move(sql),
				move(what),
				{args.Level()==ELogLevel::NoLog ? ELogLevel::Error : args.Level(), args.Tags==ELogTags::None ? ELogTags::Sql : args.Tags, (uint32)e.code().value()},
				sl
			},
			_serverMessage{ e.get_diagnostics().server_message() }
		{}
		string _serverMessage;
	};
}