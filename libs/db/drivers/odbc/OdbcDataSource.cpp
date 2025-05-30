#include "OdbcDataSource.h"
//#include "../../Framework/source/db/Database.h"
#include <jde/db/DBException.h>
#include "../../src/DBLog.h"
#include "MsSql/MsSqlSchemaProc.h"
#include "Bindings.h"
#include "OdbcRow.h"
#include "Handle.h"
#include "Utilities.h"

#define let const auto
Jde::DB::IDataSource* GetDataSource(){
	return new Jde::DB::Odbc::OdbcDataSource();
}

namespace Jde::DB::Odbc{
	constexpr ELogTags _tags = ELogTags::Sql;
	α OdbcDataSource::Disconnect()ε->void{
		BREAK;
	}
	α AllocateBindings( const HandleStatement& statement,  SQLSMALLINT columnCount )ε->vector<up<Binding>>;
	α OdbcDataSource::AtCatalog( sv catalog, SL /*sl*/ )ε->sp<IDataSource>{
		string catalogName;
		try{
			catalogName = CatalogName();
		}
		catch( const IException& ){//assume can't connect on current schema.
		}
		sp<IDataSource> ds;
		if( catalog==catalogName )
			ds = shared_from_this();
		else{
			ds = sp<IDataSource>( GetDataSource() );
			ds->SetConnectionString( CS() );
			ds->Execute( "use "+string{catalog} );
		}
		return ds;
	}
	α OdbcDataSource::AtSchema( sv /*schema*/, SL sl )ε->sp<IDataSource>{
		Critical{ sl, _tags, "Odbc doesn't support AtSchema." };
		return shared_from_this();
	}

	α OdbcDataSource::ExecDirect( string sql, const RowΛ* f, const vector<Value>* params, SL sl, bool log )Ε->uint{
		HandleStatement statement{ CS() };
		vector<SQLUSMALLINT> paramStatusArray;
		vector<up<Binding>> parameters; //keepalive
		void* pData = nullptr;
		if( params ){
			parameters.reserve( params->size() );
			SQLUSMALLINT iParameter = 0;
			for( let& param : *params ){
				auto binding = Binding::Create( param );
				pData = binding->Data();
				let size = std::max<SQLLEN>(binding->Size(),0); let bufferLength = binding->BufferLength();
				if( binding->DBType()==SQL_DATETIME )
					Trace{ _tags, "fractions={}", dynamic_cast<const BindingDateTime*>(binding.get())->_data.fraction };
				let decimals = binding->DecimalDigits();
				let result = ::SQLBindParameter( statement, ++iParameter, SQL_PARAM_INPUT, binding->CodeType, binding->DBType(), size, decimals, pData, bufferLength, &binding->Output );
				THROW_IFX(result < 0, DBException(result, move(sql), params, HandleDiagnosticRecord("SQLBindParameter", statement, SQL_HANDLE_STMT, result, sl), sl) );
				parameters.push_back( move(binding) );
			}
		}
		uint resultCount = 0;
		if( log )
			DB::Log( sql, params, sl );
		let retCode = ::SQLExecDirect( statement, (SQLCHAR*)sql.data(), static_cast<SQLINTEGER>(sql.size()) );
		switch( retCode ){
		case SQL_NO_DATA://update with no records effected...
			Debug{ _tags, "noData={}", sql };
		case SQL_SUCCESS_WITH_INFO:
			try{
				HandleDiagnosticRecord( "SQLExecDirect", statement, SQL_HANDLE_STMT, retCode );
			}
			catch( const DBException& e ){
				throw DBException{ retCode, sql, params, e.what(), sl };
			}
		case SQL_SUCCESS:{
			SQLSMALLINT columnCount{};
			if( f )
				CALL( statement, SQL_HANDLE_STMT, SQLNumResultCols(statement,&columnCount), "SQLNumResultCols" );
			if( columnCount>0 ){
				let bindings = AllocateBindings( statement, columnCount );
				OdbcRow row{ bindings };
				while( ::SQLFetch(statement)!=SQL_NO_DATA_FOUND ){
					row.Reset();
					(*f)( row );
				}
			}
			else{
				SQLLEN count;
				CALL( statement, SQL_HANDLE_STMT, SQLRowCount(statement,&count), "SQLRowCount" );
				resultCount = count;
			}
			break;
		}
		case SQL_INVALID_HANDLE:
			throw DBException( retCode, sql, params, "SQL_INVALID_HANDLE" );
			break;
		case SQL_ERROR:
			throw DBException{ retCode, sql, params, HandleDiagnosticRecord("SQLExecDirect", statement, SQL_HANDLE_STMT, retCode, sl), sl };
		default:
			throw DBException( retCode, sql, params, "Unknown error" );
		}
		return resultCount;
	}

	void OdbcDataSource::SetConnectionString( string x )ι{
		_connectionString = Ƒ( "{};APP={}", move(x), Process::ApplicationName() );
		Debug( _tags, "connectionString={}", _connectionString );
	}

	vector<up<Binding>> AllocateBindings( const HandleStatement& statement,  SQLSMALLINT columnCount )ε{
		vector<up<Binding>> bindings; bindings.reserve( columnCount );
		for( SQLSMALLINT iCol = 1; iCol <= columnCount; ++iCol ){
			SQLLEN ssType;
			CALL( statement, SQL_HANDLE_STMT, ::SQLColAttribute(statement, iCol, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &ssType), "SQLColAttribute::Concise" );

			SQLLEN bufferSize = 0;
			up<Binding> binding;
			if( ssType == SQL_CHAR || ssType == SQL_VARCHAR || ssType == SQL_LONGVARCHAR || ssType == -9/*varchar(max)?*/ || ssType == -10/*nvarchar(max)?*/ ){
				CALL( statement, SQL_HANDLE_STMT, ::SQLColAttribute(statement, iCol, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &bufferSize), "SQLColAttribute::Display" );
				if( (ssType==-9 || ssType==-10) && bufferSize==0 )
					bufferSize = (1 << 14) - 1;//TODO handle varchar(max).
				binding = mu<BindingString>( (SQLSMALLINT)ssType, ++bufferSize );
			}
			else
				binding = Binding::GetBinding( (SQLSMALLINT)ssType );
			CALL( statement, SQL_HANDLE_STMT, ::SQLBindCol(statement, iCol, (SQLSMALLINT)binding->CodeType, binding->Data(), bufferSize, &binding->Output), "SQLBindCol" );
			bindings.push_back( move(binding) );
		}
		return bindings;
	}

	α OdbcDataSource::Execute( string sql, SL sl )ε->uint{ return Select( sql, nullptr, nullptr, sl ); }
	α OdbcDataSource::Execute( string sql, const vector<Value>& parameters, SL sl)ε->uint{ return Execute(sql, &parameters, nullptr, false, sl); }
	α OdbcDataSource::Execute( string sql, const vector<Value>* pParameters, const RowΛ* f, bool /*isStoredProc*/, SL sl )ε->uint{ return ExecDirect( sql, f, pParameters, sl );  }
	α OdbcDataSource::ExecuteCo( string sql, vector<Value> p, SL sl )ι->up<IAwait>{
		return SelectCo( nullptr, move(sql), move(p), sl );
	}
	α OdbcDataSource::ExecuteCo( string sql_, vector<Value> p, bool proc_, RowΛ f, SL sl )ε->up<IAwait>{
		return mu<TPoolAwait<uint>>( [sql=move(sql_), params=move(p), sl, proc=proc_, func=f, this]()ε{
			return mu<uint>( Execute(move(sql), &params, &func, proc, sl) );
			}, "ExecuteCo", sl );
	}
	α OdbcDataSource::ExecuteNoLog( string sql, const vector<Value>* p, RowΛ* f, bool, SL sl )ε->uint{ return ExecDirect( move(sql), f, p, sl, false );  }
	α OdbcDataSource::ExecuteProcNoLog( string sql, vec<Value> v, SL sl )ε->uint{ return ExecDirect( Ƒ("{{call {} }}", move(sql)), nullptr, &v, sl, false ); }
	α OdbcDataSource::ExecuteProc( string sql, const vector<Value>& parameters, SL sl )ε->uint{ return ExecDirect( Ƒ("{{call {} }}", sql), nullptr, &parameters, sl); }
	α OdbcDataSource::ExecuteProc( string sql, const vector<Value>& parameters, RowΛ f, SL sl )ε->uint{ return Select(Ƒ("{{call {} }}", move(sql)), f, &parameters, sl); }
	α OdbcDataSource::ExecuteProcCo( string sql, vector<Value> p, SL sl )ι->up<IAwait>{
		return ExecuteCo(Ƒ("{{call {} }}", move(sql)), move(p), sl );
	}

	α OdbcDataSource::ExecuteProcCo( string sql, vector<Value> params, RowΛ f, SL sl )ε->up<IAwait>{
		return ExecuteCo( Ƒ("{{call {} }}", move(sql)), move(params), true, f, sl );
	}

	α OdbcDataSource::ServerMeta()ι->IServerMeta&{
		if( !_schemaProc )
			_schemaProc = mu<MsSql::MsSqlSchemaProc>( shared_from_this() );
		return *_schemaProc;
	}
	α OdbcDataSource::SelectNoLog( string sql, RowΛ f, const vector<Value>* p, SL sl )ε->uint{ return ExecDirect(move(sql), &f, p, sl, false); }
	α OdbcDataSource::Select( string sql, RowΛ f, const vector<Value>* p, SL sl )ε->uint{ return ExecDirect( move(sql), &f, p, sl ); }
	α OdbcDataSource::Select( Sql&& s, bool storedProc, SL sl )Ε->vector<up<IRow>> {
		vector<up<IRow>> rows;
		function<void( IRow& )> f = [&rows]( IRow& r )ι{
			rows.push_back(r.Move());
		};
		ExecDirect( storedProc ? Ƒ("{{ call {} }}", move(s.Text)) : move(s.Text), &f, &s.Params, sl );
		return rows;
	}

	α OdbcDataSource::SelectCo( ISelect* pAwait, string sql_, vector<Value>&& params_, SL sl_ )ι->up<IAwait>{
		return mu<TPoolAwait<uint>>( [this,cs=CS(),pAwait,sql=move(sql_),params=move(params_), sl=sl_]()->up<uint>{
			uint y;
			if( pAwait ){
				function<void( IRow& )ε> f = [this,pAwait](let& r){pAwait->OnRow(r);};
				y = ExecDirect( sql, &f, &params, sl );
			}
			else
				y = ExecDirect( sql, nullptr, &params, sl );
			return mu<uint>( y );
			//return y;
		});
/*		return mu<AsyncAwait>( [pAwait,sql=move(sql_),params=move(params_), sl=sl_,this]( HCoroutine h )mutable->Task
		{
			try
			{
				auto bindings = params.size() ? mu<vector<up<Binding>>>() : up<vector<up<Binding>>>{};
				if( bindings )
				{
					bindings->reserve( params.size() );
					for( let& param : params )
						bindings->push_back( Binding::Create(param) );
				}
				auto pSession =  (co_await Connect() ).SP<HandleSessionAsync>();
				DB::Log( sql, &params, sl );
				auto pStatement = ( co_await Execute(move(*pSession), move(sql), move(bindings), move(params), sl) ).SP<HandleStatementAsync>();
				if( pAwait )
					pStatement = ( co_await Fetch(move(*pStatement), pAwait) ).SP<HandleStatementAsync>();
				else
				{
					CALL( *pStatement, SQL_HANDLE_STMT, ::SQLRowCount(*pStatement, (SQLLEN*)&pStatement->_result), "SQLRowCount" );
					h.promise().get_return_object().SetResult( mu<uint>(pStatement->_result) );
				}
			}
			catch( IException& e )
			{
				h.promise().get_return_object().SetResult( e.Clone() );
			}
			h.resume();
		}, sl_);*/
	}
}