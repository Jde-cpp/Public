#include "MySqlDataSource.h"
#include <jde/db/DBException.h>
#include <jde/db/generators/Functions.h>
#include "field.h"
#include "MySqlException.h"
#include "MySqlQueryAwait.h"
#include "MySqlRow.h"
#include "MySqlServerMeta.h"
#include "../../src/DBLog.h"

#ifndef NDEBUG
	#ifndef _GLIBCXX_DEBUG
			#error "_GLIBCXX_DEBUG must be defined to compile this code."
	#endif
#endif

#define let const auto

Jde::DB::IDataSource* GetDataSource(){
	return new Jde::DB::MySql::MySqlDataSource();
}

namespace Jde::DB::MySql{
//	constexpr ELogTags _tags{ ELogTags::Sql };
//	using mysqlx::SessionOption;
	α MySqlDataSource::Params::HasOut()Ι->bool{ return OutValue!=EValue::Null; }
	namespace mysql = boost::mysql;
	α toString( const mysql::connect_params& cs )ι->string{
		return Ƒ( "'{}@{}:{}/{}' pwd:'{}' collation:{}, ssl:{}, multi:{}",
			cs.username,
			cs.server_address.hostname(),
			cs.server_address.port(),
			cs.database,
			cs.password.empty() ? "<empty>" : "<set>",
			cs.connection_collation,
			underlying(cs.ssl),
			cs.multi_queries
		);
	}
	struct Session final{
		Session( const mysql::connect_params& cs, SL sl )ε:
			Conn{ _ctx }{
			Logging::LogOnce( SRCE_CUR, ELogTags::DBDriver, "mysql::connect_params: {}", toString(cs) );
			try{
				Conn.connect( cs );
			}
			catch( mysql::error_with_diagnostics& e ){
				throw MySqlException{ toString(cs), move(e), sl, ELogTags::DBDriver, ELogLevel::Critical };
			}
		}
	private:
    asio::io_context _ctx;
	public:
		mysql::any_connection Conn;
	};

	α MySqlDataSource::SetConfig( const jobject& config )ε->void{
		auto host = Json::FindSV( config, "host" ).value_or( "localhost" );
		_cs.server_address = mysql::any_address{ mysql::host_and_port{string{host}, Json::FindNumber<PortType>(config, "port").value_or(3306)} };
		_cs.username = Json::AsSV( config, "username" );
		_cs.password = Json::AsSV( config, "password" );
		_cs.database = Json::AsSV( config, "schema" );
		_cs.connection_collation = 45; //utf8mb4_general_ci
		_cs.ssl = mysql::ssl_mode::disable;
		_cs.multi_queries = true;
	}

	α MySqlDataSource::Execute( DB::Sql&& sql, SL sl, Params exeParams )ε->uint{
		if( sql.IsProc )
			sql.Text = Ƒ( "call {}", move(sql.Text) );
		if( log )
			DB::Log( sql, sl );

		auto session = Session( _cs, sl );
		vector<mysql::field_view> params; params.reserve( sql.Params.size() );
		for( uint i=0; i<sql.Params.size()+(exeParams.HasOut() ? -1 : 0); ++i )
			params.push_back( ToField(sql.Params[i], sl) );
		if( exeParams.HasOut() )
			params.push_back( mysql::field_view{0ul} );
   	mysql::results result;
		mysql::statement stmt;
		try{
			if( exeParams.Function && exeParams.HasOut() ){
				ASSERT( sql.Params.size() );
				stmt = session.Conn.prepare_statement( move(sql.Text) );
				session.Conn.execute( stmt.bind(params.begin(), params.end()), result );
				auto view = result.out_params();
				ASSERT( view.size() );
				(*exeParams.Function)( ToRow(result.out_params(), sl) );
			}
			else if( sql.Params.empty() )
				session.Conn.execute( sql.Text, result );
			else
				session.Conn.execute( sql.EmbedParams(), result );
		}
		catch( mysql::error_with_diagnostics& e ){
			throw MySqlException{ move(sql.Text), move(e), sl, ELogTags::DBDriver };
		}
		if( exeParams.Function && result.has_value() ){
			for( auto&& row : result.rows() )
				(*exeParams.Function)( ToRow(row, sl) );
		}
		if( stmt.valid() )
			session.Conn.close_statement( stmt );
		session.Conn.close();

		let y = result.has_value()
			? exeParams.Sequence ? result.last_insert_id() : result.affected_rows()
			: 0;
		ASSERT_DESC( !exeParams.Sequence || y, "MySql should return last insert id." );
		return y;
	}

	α MySqlDataSource::AtCatalog( sv catalog, SL sl )ε->sp<IDataSource>{
		Critical{ _tags, "MySql doesn't have catalogs." };
		return shared_from_this();
	}

	α MySqlDataSource::SchemaNameConfig( SL sl )ι->string{ return _cs.database.empty() ? string{} : _cs.database; }

	α MySqlDataSource::AtSchema( sv schema, SL sl )ε->sp<IDataSource>{
		string schemaName;
		try{
			schemaName = SchemaName();
		}
		catch( const IException& e ){//assume can't connect on current schema.
		}
		sp<MySqlDataSource> pDataSource;
		if( schema==schemaName )
			pDataSource = dynamic_pointer_cast<MySqlDataSource>( shared_from_this() );
		else{
			pDataSource = sp<MySqlDataSource>( (MySqlDataSource*)GetDataSource() );
			pDataSource->_cs = _cs;
			pDataSource->_cs.database = schema;
		}
		return pDataSource;
	}

	α MySqlDataSource::ExecuteSync( Sql&& sql, SL sl )ε->uint{
		return Execute( move(sql), sl );
	}
	α MySqlDataSource::ExecuteScalerSync( Sql&& sql, EValue outValue, SL sl )ε->Value{
		Value y;
		RowΛ f = [&]( Row&& r )->void {
			THROW_IFSL( r.Size()==0, "Query did not return any {}.", empty(outValue) ? "rows" : "out params" );
			y = move(r[0]);
		};
		Execute( move(sql), sl, {&f, outValue} );
		return y;
	}

	α MySqlDataSource::InsertSeqSyncUInt( DB::InsertClause&& insert, SL sl )ε->uint{
		insert.Add( {}, 0ul );
		uint y{};
		RowΛ f = [&y]( Row&& r ){ y = r.GetUInt(0); };
		Execute( insert.Move(), sl, {.Function=&f, .OutValue=EValue::UInt64} );
		return y;
	}


	α MySqlDataSource::ServerMeta()ι->IServerMeta&{
		MySqlServerMeta a( shared_from_this() );
		if( !_schemaProc )
			_schemaProc = mu<MySqlServerMeta>( shared_from_this() );
		return *_schemaProc;
	}
	α MySqlDataSource::Select( Sql&& s, SL sl )ε->vector<Row>{
		vector<Row> rows;
		RowΛ f = [&rows]( Row&& r ){
			rows.push_back( move(r) );
		};
		Execute( move(s), sl, {&f} );
		return rows;
	}
	α MySqlDataSource::Select( Sql&& s, RowΛ f, SL sl )ε->uint{
		return Execute( move(s), sl, {&f} );
	}

	α MySqlDataSource::Query( Sql&& sql, bool outParams, SL sl )ε->QueryAwait{
		return QueryAwait{ mu<MySqlQueryAwait>(dynamic_pointer_cast<MySqlDataSource>(shared_from_this()), move(sql), outParams, sl), sl };
	}

	α MySqlDataSource::ExecuteNoLog( Sql&& sql, SL sl )ε->uint{
		return Execute( move(sql), sl, {.Log=false} );
	}
}