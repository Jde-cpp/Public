#include "MySqlDataSource.h"
#include <jde/db/Database.h>
#include <jde/db/DBException.h>
#include "MySqlRow.h"
#include "MySqlServerMeta.h"
#include "../../DBLog.h"

#define let const auto

Jde::DB::IDataSource* GetDataSource(){
	return new Jde::DB::MySql::MySqlDataSource();
}

namespace Jde::DB::MySql{
	constexpr ELogTags _tags{ ELogTags::Sql };

	using mysqlx::SessionOption;
	α Session( str cs, SRCE )ε->mysqlx::Session{
		try{
			return mysqlx::Session{ cs };
		}
		catch( std::exception& e ){
			Exception e2{ SRCE_CUR, move(e), "Could not create mysql session" };
			e2.Push( sl );
			e2.SetLevel( ELogLevel::Critical );
			e2.Throw();
		}
	}

	α ToMySqlValue( const Value& dataValue )ε->mysqlx::Value{
		switch( dataValue.Type() ){
		using enum EValue;
			case Null: return mysqlx::Value();
			case String: return mysqlx::Value( dataValue.get_string() );
			case Bool: return mysqlx::Value( dataValue.get_bool() );
			case Int8: return mysqlx::Value( dataValue.get_int8() );
			case Int32: return mysqlx::Value( dataValue.get_int32() );
			case Int64: return mysqlx::Value( dataValue.get_int() );
			case UInt32: return mysqlx::Value( dataValue.get_uint32() );
			case UInt64: return mysqlx::Value( dataValue.get_uint() );
			case Double: return mysqlx::Value( dataValue.get_double() );
			case Time:{
				let value = dataValue.get_time();
				const Jde::DateTime date{ value };
				auto stringValue = date.ToIsoString();
				stringValue = Str::Replace( stringValue, "T", " " );
				stringValue = Str::Replace( stringValue, "Z", "" );
				uint nanos = value.time_since_epoch().count();
				constexpr uint NanosPerSecond = std::chrono::nanoseconds(1s).count();
				const double remainingNanos = nanos%NanosPerSecond;
				double fraction = remainingNanos/NanosPerSecond;
				let rounded = std::round( fraction*1000000 );
				let fraction2 = rounded/1000000;
				std::ostringstream os;
				os << std::setprecision(6) << std::fixed << fraction2;
				auto fractionString = os.str().substr(1);

				string value2 = Ƒ( "{}{}", stringValue, fractionString );//:.6
				if( value2.find('e')!=string::npos )
					Error{ _tags, "{} has {} for {} returning {}, nanos={}"sv, value2, fractionString, fraction, value2, value.time_since_epoch().count() };//2019-12-13 20:43:04.305e-06 has .305e-06 for 9.30500e-06 returning 2019-12-13 20:43:04.305e-06, nanos=1576269784000009305
				return mysqlx::Value( value2 );
			}
		}
		throw Exception{ SRCE_CUR, ELogLevel::Debug, "{} dataValue not implemented", dataValue.TypeName() };
		return mysqlx::Value( "compiler remove warning noop" );
	}

//https://dev.mysql.com/doc/refman/8.0/en/c-api-prepared-call-statements.html
	α Execute( str cs, string&& sql, const vector<Value>* pParameters, const RowΛ* pFunction, bool proc, SL sl, bool log=true )ε->uint{
		ASSERT( cs.size() );
		let fullSql = proc ? Ƒ( "call {}", move(sql) ) : move( sql );
		if( log )
			DB::Log( fullSql, pParameters, sl );
		mysqlx::Session session = Session( cs );
		auto statement = session.sql( fullSql );
		if( pParameters ){
			for( let& parameter : *pParameters )
				statement.bind( ToMySqlValue(parameter) );
		}
		mysqlx::SqlResult result;
		try{
			result = statement.execute();
			if( pFunction && result.hasData() ){
				auto rows = result.fetchAll();
				for( const mysqlx::Row& row : rows ){
					MySqlRow r{ row };
					(*pFunction)( r );
				}
			}
		}
		catch( ::mysqlx::Error& e ){
			throw DBException{ fullSql, pParameters, e.what(), sl };
		}
		try{
			return proc || result.hasData() ? 0 : result.getAffectedItemsCount();
		}
		catch( ::mysqlx::Error& e ){//Only available after end of query execute
			return 0;
		}
	}
	α Select( str cs, str sql, RowΛ f, const vector<Value>* pValues, SL sl, bool log=true )ε->uint{
		mysqlx::Session session = Session( cs );
		auto statement = session.sql( sql );
		if( pValues ){
			for( let& value : *pValues )
				statement.bind( ToMySqlValue(value) );
		}
		try{
			if( log )
				DB::Log( sql, pValues, sl );
			auto result = statement.execute();
			std::list<mysqlx::Row> rows = result.fetchAll();//
			for( mysqlx::Row& row : rows ){
				MySqlRow r{ row };
				f( r );
			}
			return rows.size();
		}
		catch( ::mysqlx::Error& e ){
			throw DBException{ sql, pValues, e.what(), sl };
		}
	}

	α MySqlDataSource::AtCatalog( sv catalog, SL sl )ε->sp<IDataSource>{
		Critical{ _tags, "MySql doesn't have catalogs." };
		return shared_from_this();
	}
	α MySqlDataSource::SchemaNameConfig( SL sl )ε->string{
		uint start = _connectionString.find_last_of('/');
		uint end = _connectionString.find_last_of('?');
		return end>start && start!=string::npos ? _connectionString.substr( start+1, end-start-1 ) : string{};
	}

	α MySqlDataSource::AtSchema( sv schema, SL sl )ε->sp<IDataSource>{
		string schemaName;
		try{
			schemaName = SchemaName();
		}
		catch( const IException& e ){//assume can't connect on current schema.
		}
		sp<IDataSource> pDataSource;
		if( schema==schemaName )
			pDataSource = shared_from_this();
		else{
			pDataSource = sp<IDataSource>( GetDataSource() );
			let csSpecified = schemaName.size() && _connectionString.find( schemaName )!=string::npos;
			let cs = csSpecified
				? Str::Replace( _connectionString, SchemaName(), schema )
				: _connectionString.substr( 0, _connectionString.find_last_of('/')+1 )+string{schema}+_connectionString.substr( _connectionString.find_last_of('?') );
			pDataSource->SetConnectionString( cs );
		}
		return pDataSource;
	}


	α MySqlDataSource::Execute( string sql, SL sl )ε->uint{
		return Execute( move(sql), nullptr, nullptr, false, sl );
	}
	α MySqlDataSource::Execute( string sql, const vector<Value>& parameters, SL sl )ε->uint{
		return Execute( move(sql), &parameters, nullptr, false, sl );
	}
	α MySqlDataSource::Execute( string sql, const vector<Value>* pParams, const RowΛ* f, bool proc, SL sl )ε->uint{
		return MySql::Execute( CS(), move(sql), pParams, f, proc, sl );
	}

	α MySqlDataSource::ExecuteProc( string sql, const vector<Value>& parameters, SL sl )ε->uint{
		return MySql::Execute( CS(), move(sql), &parameters, nullptr, true, sl );
	}
	α MySqlDataSource::ExecuteProc( string sql, const vector<Value>& parameters, RowΛ f, SL sl )ε->uint{
		return MySql::Execute( CS(), move(sql), &parameters, &f, true, sl );
	}

	α MySqlDataSource::ServerMeta()ι->IServerMeta&{
		if( !_pSchemaProc )
			_pSchemaProc = mu<MySqlServerMeta>( shared_from_this() );
		return *_pSchemaProc;
	}
	α MySqlDataSource::Select( Sql&& s, SL sl )Ε->vector<up<IRow>>{
		vector<up<IRow>> rows;
		RowΛ f = [&rows]( IRow& r ){
			rows.push_back(r.Move());
		};
		MySql::Select( CS(), move(s.Text), f, &s.Params, sl );
		return rows;
	}
	α MySqlDataSource::Select( string sql, RowΛ f, const vector<Value>* pValues, SL sl )ε->uint{
		return MySql::Select( CS(), move(sql), f, pValues, sl );
	}

	α MySqlDataSource::SelectCo( ISelect* pAwait, string sql_, vector<Value>&& params_, SL sl )ι->up<IAwait>{
		return mu<PoolAwait>( [pAwait, sql{move(sql_)}, params=move(params_), sl, this]()ε{
			auto rowΛ = [pAwait]( const IRow& r )ε{ pAwait->OnRow(r); };
			Select( sql, rowΛ, &params, sl );
		});
	}
	α MySqlDataSource::ExecuteCo( string sql_, const vector<Value> p, bool proc_, SL sl )ι->up<IAwait>{
		return mu<TPoolAwait<uint>>( [sql=move(sql_), params=move(p), sl, proc=proc_, this]()ε{
			return mu<uint>( Execute(move(sql), &params, nullptr, proc, sl) );
		});
	}
	α MySqlDataSource::ExecuteCo( string sql_, vector<Value> p, bool proc_, RowΛ f, SL sl )ε->up<IAwait>{
		return mu<TPoolAwait<uint>>( [sql=move(sql_), params=move(p), sl, proc=proc_, func=f, this]()ε{
			return mu<uint>( Execute(move(sql), &params, &func, proc, sl) );
		}, "ExecuteCo", sl );
	}
	α MySqlDataSource::ExecuteNoLog( string sql, const std::vector<Value>* pParameters, RowΛ* f, bool proc, SL sl )ε->uint{
		return MySql::Execute( CS(), move(sql), pParameters, f, proc, sl, false );
	}
	α MySqlDataSource::ExecuteProcNoLog( string sql, const std::vector<Value>& parameters, SL sl )ε->uint{
		return MySql::Execute( CS(), move(sql), &parameters, nullptr, true, sl, false );
	}
	α MySqlDataSource::SelectNoLog( string sql, RowΛ f, const std::vector<Value>* pValues, SL sl )ε->uint{
		return MySql::Select( CS(), move(sql), f, pValues, sl, false );
	}
}