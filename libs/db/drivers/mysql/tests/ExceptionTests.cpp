#include <gtest/gtest.h>
#include <boost/asio/error.hpp>
#include <boost/mysql/client_errc.hpp>          //MySqlException.h reaches for `mysql::` without including boost itself -
#include <boost/mysql/common_server_errc.hpp>   //the driver's own TUs get it from their pc.h, which this suite does not share.
#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/diagnostics.hpp>
#include <jde/db/generators/Sql.h> //DB::Sql, for constructing the exception itself.
#include <boost/mysql/mariadb_server_errc.hpp>
#include <boost/mysql/mysql_server_errc.hpp>
#include "../src/MySqlException.h"

#define let const auto

namespace Jde::DB::MySql::Tests{
	//Classification only - the error_codes are synthesised, nothing connects to a server.  #9: the two branches were
	//inverted, so a downed MySQL reported None (500 "server fault") while a bound-param-count mismatch reported
	//Connection (503 "transient - retryable").  What HttpStatus does with each is DBException::HttpStatus.

	//The real connection failures: asio/system/netdb/ssl categories, none of which is a mysql category.
	TEST( ExceptionTests, TransportFailuresAreConnection ){
		using boost::system::error_code;
		EXPECT_EQ( ToDbError(error_code{(int)std::errc::connection_refused, boost::system::system_category()}), EDbError::Connection );
		EXPECT_EQ( ToDbError(error_code{(int)std::errc::connection_reset, boost::system::system_category()}), EDbError::Connection );
		EXPECT_EQ( ToDbError(error_code{(int)std::errc::broken_pipe, boost::system::system_category()}), EDbError::Connection );
		EXPECT_EQ( ToDbError(make_error_code(boost::asio::error::eof)), EDbError::Connection );
		EXPECT_EQ( ToDbError(make_error_code(boost::asio::error::host_not_found)), EDbError::Connection );
	}

	//client_errc splits: no usable session -> Connection; caller misused the library -> None (a 500, like any other
	//fault of ours).  The whole family used to answer Connection.
	TEST( ExceptionTests, ClientErrcSplitsTransportFromMisuse ){
		using errc = mysql::client_errc;
		for( let& e : {errc::incomplete_message, errc::server_unsupported, errc::sequence_number_mismatch,
				errc::unknown_auth_plugin, errc::bad_handshake_packet_type, errc::not_connected} )
			EXPECT_EQ( ToDbError(make_error_code(e)), EDbError::Connection ) << (int)e;

		for( let& e : {errc::wrong_num_params, errc::metadata_check_failed, errc::num_resultsets_mismatch,
				errc::format_string_invalid_syntax, errc::format_arg_not_found, errc::pool_not_running} )
			EXPECT_EQ( ToDbError(make_error_code(e)), EDbError::None ) << (int)e;
	}

	//Server diagnostics the switch did not cover.  "no such table"/"unknown column"/"no such proc" are Syntax on the
	//sqlite and odbc drivers, so MySQL agreeing with its siblings is the point.
	TEST( ExceptionTests, ServerCodesMatchTheOtherDrivers ){
		using errc = mysql::common_server_errc;
		EXPECT_EQ( ToDbError(make_error_code(errc::er_no_such_table)), EDbError::Syntax );      //1146
		EXPECT_EQ( ToDbError(make_error_code(errc::er_bad_field_error)), EDbError::Syntax );    //1054
		EXPECT_EQ( ToDbError(make_error_code(errc::er_sp_does_not_exist)), EDbError::Syntax );  //1305
		EXPECT_EQ( ToDbError(make_error_code(errc::er_parse_error)), EDbError::Syntax );        //1064, already mapped
		EXPECT_EQ( ToDbError(make_error_code(errc::er_server_shutdown)), EDbError::Connection );//1053

		//mysql-category numbers, gated on the category because MariaDB reuses the range.
		let mysqlCode = []( int v ){ return boost::system::error_code{ v, mysql::get_mysql_server_category() }; };
		EXPECT_EQ( ToDbError(mysqlCode(mysql::mysql_server_errc::er_check_constraint_violated)), EDbError::Check );      //3819
		EXPECT_EQ( ToDbError(mysqlCode(mysql::mysql_server_errc::er_client_interaction_timeout)), EDbError::Connection );//4031
	}

	//The classifications that were already right stay right.
	TEST( ExceptionTests, ExistingServerMappingsHold ){
		using errc = mysql::common_server_errc;
		EXPECT_EQ( ToDbError(make_error_code(errc::er_dup_entry)), EDbError::Duplicate );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_no_referenced_row_2)), EDbError::ForeignKey );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_bad_null_error)), EDbError::NotNull );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_lock_deadlock)), EDbError::Deadlock );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_lock_wait_timeout)), EDbError::Timeout );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_access_denied_error)), EDbError::Permission );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_signal_exception)), EDbError::App );
		EXPECT_EQ( ToDbError(make_error_code(errc::er_unknown_error)), EDbError::None ); //unmapped server code stays None.
	}


	//#49: the sync Execute wrapped every server error as {Critical, DBDriver}, so a duplicate key or an fk violation
	//was logged as a driver emergency - unlike MySqlQueryAwait and the sqlite/odbc drivers, which all use these
	//defaults.  The fix is to stop passing args there, so what the defaults are is now load-bearing.
	TEST( ExceptionTests, DefaultSeverityIsErrorNotCritical ){
		using errc = mysql::common_server_errc;
		const mysql::error_with_diagnostics duplicate{ make_error_code(errc::er_dup_entry), mysql::diagnostics{} };

		const MySqlException defaulted{ DB::Sql{"insert into t values(1)"}, duplicate, SRCE_CUR };
		EXPECT_EQ( defaulted.Level(), ELogLevel::Error );
		EXPECT_EQ( defaulted.Tags, ELogTags::Sql );
		EXPECT_EQ( defaulted.Error, EDbError::Duplicate ); //classification never depended on the args either way.

		//the connect failure asks for Critical explicitly and has to keep getting it - that one is a driver emergency.
		const MySqlException connect{ "mysql://host/db", duplicate, {ELogLevel::Critical, ELogTags::DBDriver}, SRCE_CUR };
		EXPECT_EQ( connect.Level(), ELogLevel::Critical );
		EXPECT_EQ( connect.Tags, ELogTags::DBDriver );
	}
}
