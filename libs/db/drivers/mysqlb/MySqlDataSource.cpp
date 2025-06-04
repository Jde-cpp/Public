#include "MySqlDataSource.h"
#include <jde/db/DBException.h>
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
	//return nullptr; //TODO!!!
}

namespace Jde::DB::MySql{
//	constexpr ELogTags _tags{ ELogTags::Sql };

//	using mysqlx::SessionOption;
	namespace mysql = boost::mysql;
	struct Session final{
		Session( const mysql::connect_params& cs, SL sl )ε:
			Conn{ _ctx }{
			try{
				Conn.connect( cs );
			}
			catch( mysql::error_with_diagnostics& e ){
				throw MySqlException{ ELogTags::DBDriver, sl, ELogLevel::Critical, move(e) };
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

	α MySqlDataSource::Execute( DB::Sql&& sql, const RowΛ* f, bool prepare, SL sl, bool log )ε->uint{
		let fullSql = sql.IsProc ? Ƒ( "call {}", move(sql.Text) ) : move( sql.Text );
		if( log )
			DB::Log( fullSql, &sql.Params, sl );

		auto session = Session( _cs, sl );
		vector<mysql::field_view> params; params.reserve( sql.Params.size() );
		for( let& param : sql.Params )
			params.push_back( ToField(param, sl) );
   	mysql::results result;
		mysql::statement stmt;
		try{
			if( prepare && sql.Params.size() ){
				stmt = session.Conn.prepare_statement( fullSql );
				session.Conn.execute( stmt.bind(params.begin(), params.end()), result );
			}
			else if( sql.Params.empty() )
				session.Conn.execute( fullSql, result );
			else{
				string parameterizedSql; parameterizedSql.reserve( fullSql.size()*2 );
				uint iStart=0;
				for( uint i=fullSql.find('?'), iParam=0; iParam<sql.Params.size() && i<fullSql.size(); iStart=i+1, i=fullSql.find('?', i+1) ){
					parameterizedSql.append( fullSql, iStart, i-iStart );
					auto& param = sql.Params[iParam++];
					if( param.Type()==EValue::String )
						parameterizedSql.append( '\''+Str::Replace(param.move_string(), "\'", "''")+'\'' );
					else if( param.Type()==EValue::Null )
						parameterizedSql.append( "null" );
					else
						parameterizedSql.append( param.ToString() );
				}
				if( iStart<fullSql.size() )
					parameterizedSql.append( fullSql, iStart, fullSql.size()-iStart );
				Trace{ _tags, "{}", parameterizedSql };
				session.Conn.execute( parameterizedSql, result );
			}
		}
		catch( mysql::error_with_diagnostics& e ){
			throw MySqlException{ ELogTags::DBDriver, sl, ELogLevel::Error, move(e) };
		}
		if( f && result.has_value() ){
			auto rows = result.rows();
			for( auto&& row : rows )
				(*f)( ToRow(row, sl) );
		}
		if( stmt.valid() )
			session.Conn.close_statement( stmt );
		session.Conn.close();

		return result.has_value() ? result.affected_rows() : 0;
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

	α MySqlDataSource::Execute( Sql&& sql, bool prepare, SL sl )ε->uint{
		return Execute( move(sql), nullptr, prepare, sl, true );
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
		Execute( move(s), &f, false, sl, true );
		return rows;
	}
	α MySqlDataSource::Select( Sql&& s, RowΛ f, SL sl )ε->uint{
		return Execute( move(s), &f, false, sl, true );
	}

	α MySqlDataSource::Query( Sql&& sql, SL sl )ε->QueryAwait{
		return QueryAwait{ mu<MySqlQueryAwait>(dynamic_pointer_cast<MySqlDataSource>(shared_from_this()), move(sql), sl), sl };
	}

	α MySqlDataSource::ExecuteNoLog( Sql&& sql, RowΛ* f, SL sl )ε->uint{
		return Execute( move(sql), f, true, sl, false );
	}
}