#include "MySqlQueryAwait.h"
#include <boost/asio/co_spawn.hpp>
#include "field.h"
#include "MySqlRow.h"
#include "MySqlException.h"
#include "../../src/DBLog.h"

#define let const auto

namespace Jde::DB::MySql{
	MySqlQueryAwait::MySqlQueryAwait( sp<MySqlDataSource> ds, Sql&& s, SL sl )ι:
		TAwait<Result>{ sl },
		_ctx{ Executor() },
		_ds{ ds },
		_sql{ move(s) }
	{}

	α MySqlQueryAwait::Main()ι->asio::awaitable<void>{
		vector<mysql::field_view> params; params.reserve( _sql.Params.size() );
		for( let& param : _sql.Params )
			params.push_back( ToField(param, _sl) );

		mysql::any_connection conn( co_await asio::this_coro::executor );
		co_await conn.async_connect( _ds->ConnectionParams() );
		if( _sql.IsProc )
			_sql.Text = Ƒ( "call {}", move(_sql.Text) );
		if( log )
			DB::Log( _sql, _sl );
    auto stmt = co_await conn.async_prepare_statement( _sql.Text );

		mysql::results mySqlResult;
		co_await conn.async_execute( stmt.bind(params.begin(), params.end()), mySqlResult );
		Result result;
		if( mySqlResult.has_value() ){
			for( auto&& row : mySqlResult.rows() )
				result.Rows.push_back( ToRow(row, _sl) );
			result.RowsAffected = mySqlResult.affected_rows();
		}
 		co_await conn.async_close_statement( stmt );
    co_await conn.async_close();
		if( mySqlResult.has_value() )
			Trace{ ELogTags::Test, "MySqlQueryAwait::Main: RowsAffected: {} rows: {}.", result.RowsAffected, result.Rows.size() };
		else
			Trace{ ELogTags::Test, "MySqlQueryAwait::Main: No rows affected." };
		Resume( move(result) );
	}
	α MySqlQueryAwait::Suspend()ι->void{
		asio::co_spawn(
			*_ctx,
			[this]{ return Main(); },
			[this]( std::exception_ptr e ){
				if( !e )
					return;
				try{
					std::rethrow_exception( e );
				}
				catch( mysql::error_with_diagnostics& e ){
					ResumeExp( MySqlException{_tags, _sl, ELogLevel::Error, move(e), move(_sql.Text)} );
				}
				catch( IException& e ){
					ResumeExp( move(e) );
				}
				catch( ... ){
					ResumeExp( move(*IException::FromExceptionPtr(e)) );
				}
			}
    );
		Execution::Run();
	}
}