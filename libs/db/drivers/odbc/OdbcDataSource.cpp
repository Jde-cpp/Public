#include "OdbcDataSource.h"
#include <jde/db/DBException.h>
#include "../../src/DBLog.h"
#include "MsSql/MsSqlSchemaProc.h"
#include "OdbcQueryAwait.h"
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
		sp<OdbcDataSource> ds;
		if( catalog==catalogName )
			ds = dynamic_pointer_cast<OdbcDataSource>( shared_from_this() );
		else{
			ds = sp<OdbcDataSource>( (OdbcDataSource*)GetDataSource() );
			ds->SetConnectionString( _connectionString );
			ds->Execute( {"use "+string{catalog}} );
		}
		return ds;
	}
	α OdbcDataSource::AtSchema( sv /*schema*/, SL sl )ε->sp<IDataSource>{
		Critical{ sl, _tags, "Odbc doesn't support AtSchema." };
		return shared_from_this();
	}

	α OdbcDataSource::ExecDirect( Sql&& sql, SL sl, Params&& params )Ε->uint{
		HandleStatement statement{ _connectionString };
		vector<SQLUSMALLINT> paramStatusArray;
		vector<up<Binding>> parameters; parameters.reserve( sql.Params.size() ); //keepalive
		for( let& param : sql.Params ){
			auto binding = Binding::Create( param );
			void* pData = binding->Data();
			let size = std::max<SQLLEN>(binding->Size(),0);
			let bufferLength = binding->BufferLength();
			let decimals = binding->DecimalDigits();
			let paramType = params.HasOut() && parameters.size()+1==sql.Params.size() ? SQL_PARAM_OUTPUT : SQL_PARAM_INPUT;
			let dbType = binding->DBType();
			let result = ::SQLBindParameter( statement, parameters.size()+1, paramType, binding->CodeType, dbType, size, decimals, pData, bufferLength, &binding->Output );
			THROW_IFX( result < 0, DBException(result, move(sql), HandleDiagnosticRecord("SQLBindParameter", statement, SQL_HANDLE_STMT, result, sl), sl) );
			parameters.push_back( move(binding) );
		}

		/*if( prepare ){
			HandleStatement statement{ CS() };
			let retCode = ::SQLPrepare( statement, (SQLCHAR*)sql.Text.data(), static_cast<SQLINTEGER>(sql.Text.size()) );
			THROW_IFX( retCode < 0, DBException( retCode, sql.Text, &sql.Params, HandleDiagnosticRecord( "SQLPrepare", statement, SQL_HANDLE_STMT, retCode, sl ), sl ) );
		}*/
		if( sql.IsProc )
			sql.Text = Ƒ( "{{ call {} }}", move(sql.Text) );
		uint resultCount{};
		if( params.Log )
			DB::Log( sql, sl );
		let retCode = ::SQLExecDirect( statement, (SQLCHAR*)sql.Text.data(), static_cast<SQLINTEGER>(sql.Text.size()) );
		switch( retCode ){
		case SQL_NO_DATA://update with no records effected...
			Debug{ _tags, "noData={}", sql.Text };
		case SQL_SUCCESS_WITH_INFO:
			try{
				HandleDiagnosticRecord( "SQLExecDirect", statement, SQL_HANDLE_STMT, retCode );
			}
			catch( const DBException& e ){
				throw DBException{ retCode, move(sql), e.what(), sl };
			}
		case SQL_SUCCESS:{
			if( params.HasOut() && params.Function ) //if not getting value, make sure nocount on
				( *params.Function )( Row{{(*parameters.rbegin())->GetValue()}} );

			SQLSMALLINT columnCount{};
			if( params.Function )
				CALL( statement, SQL_HANDLE_STMT, SQLNumResultCols(statement,&columnCount), "SQLNumResultCols" );
			if( columnCount>0 ){
				let bindings = AllocateBindings( statement, columnCount );
				OdbcRow row{ bindings };
				while( ::SQLFetch(statement)!=SQL_NO_DATA_FOUND ){
					row.Reset();
					(*params.Function)( row.ToRow() );
					++resultCount;
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
			throw DBException( retCode, move(sql), "SQL_INVALID_HANDLE" );
			break;
		case SQL_ERROR:
			throw DBException{ retCode, move(sql), HandleDiagnosticRecord("SQLExecDirect", statement, SQL_HANDLE_STMT, retCode, sl), sl };
		default:
			throw DBException( retCode, move(sql), "Unknown error" );
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
			if( ssType == SQL_CHAR || ssType == SQL_VARCHAR || ssType == SQL_LONGVARCHAR || ssType==SQL_VARBINARY || ssType == -9/*varchar(max)?*/ || ssType == -10/*nvarchar(max)?*/ ){
				CALL( statement, SQL_HANDLE_STMT, ::SQLColAttribute(statement, iCol, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &bufferSize), "SQLColAttribute::Display" );
				if( (ssType==-9 || ssType==-10) && bufferSize==0 )
					bufferSize = (1 << 14) - 1;//TODO handle varchar(max).
				if( ssType==SQL_VARBINARY )
					binding = mu<BindingBinary>( ++bufferSize );
				else
					binding = mu<BindingString>( (SQLSMALLINT)ssType, ++bufferSize );
			}
			else
				binding = Binding::GetBinding( (SQLSMALLINT)ssType );
			CALL( statement, SQL_HANDLE_STMT, ::SQLBindCol(statement, iCol, (SQLSMALLINT)binding->CodeType, binding->Data(), bufferSize, &binding->Output), "SQLBindCol" );
			bindings.push_back( move(binding) );
		}
		return bindings;
	}

	α OdbcDataSource::ExecuteSync( Sql&& sql, SL sl )ε->uint{
		return ExecDirect( move(sql), sl, {} );
	}
	α OdbcDataSource::ExecuteNoLog( Sql&& sql, SL sl )ε->uint{
		return ExecDirect( move(sql), sl, Params{.Log=false} );
	}

	α OdbcDataSource::ServerMeta()ι->IServerMeta&{
		if( !_schemaProc )
			_schemaProc = mu<MsSql::MsSqlSchemaProc>( shared_from_this() );
		return *_schemaProc;
	}

	α OdbcDataSource::Select( Sql&& sql, RowΛ f, SL sl )ε->uint{
		return ExecDirect( move(sql), sl, {.Function=&f} );
	}
	α OdbcDataSource::Select( Sql&& s, RowΛ f, bool outParams, SL sl )ε->uint{
		return ExecDirect( move(s), sl, {.Function=&f, .OutValue=outParams ? EValue::UInt64 : EValue::Null} );
	}

	α OdbcDataSource::Select( Sql&& s, SL sl )ε->vector<Row>{
		vector<Row> rows;
		function<void( Row&& )> f = [&rows]( Row&& r )ι{
			rows.push_back( move(r) );
		};
		ExecDirect( move(s), sl, {.Function=&f} );
		return rows;
	}

	α OdbcDataSource::ExecuteScalerSync( Sql&& sql, EValue outValue, SL sl )ε->Value{
		Value y;
		RowΛ f = [&]( Row&& r )->void {
			THROW_IFSL( r.Size()==0, "Query did not return any {}.", empty(outValue) ? "rows" : "out params" );
			y = move(r[0]);
		};
		ExecDirect( move(sql), sl, {.Function=&f} );
		return y;
	}

	α OdbcDataSource::SetConfig( const jobject& config )ε->void{
		SetConnectionString( Json::AsString( config, "connectionString") );
	}

	α OdbcDataSource::Query( Sql&& sql, bool outParams, SL sl )ε->QueryAwait {
		return QueryAwait{ mu<OdbcQueryAwait>( dynamic_pointer_cast<OdbcDataSource>(shared_from_this()), move(sql), outParams, sl) };
	}

	α OdbcDataSource::InsertSeqSyncUInt( InsertClause&& insert, SL sl )ε->uint{
		insert.Add( (uint)0 );
		uint newId;
		RowΛ f = [&newId]( Row&& r ){ newId=r[0].get_number<uint>(); };
		ExecDirect( insert.Move(), sl, {&f,EValue::UInt64} );
		return newId;
	}
}