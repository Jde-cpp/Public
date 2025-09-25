#include "Handle.h"
#include <sql.h>
#include <sqlext.h>
#include <jde/db/DBException.h>
#include "Utilities.h"

#define let const auto
namespace Jde::DB::Odbc{
	constexpr ELogTags _tags{ ELogTags::App };
	SQLHENV _hEnv{};

	Ω getEnvHandle()ε->SQLHENV{
		if( !_hEnv){
			//TRACE( "Creating _hEnv" );
			SQLHENV hEnv;
			CALL( nullptr, SQL_HANDLE_ENV, ::SQLSetEnvAttr(nullptr, SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER)SQL_CP_ONE_PER_HENV, 0), "SQLSetEnvAttr(SQL_ATTR_CONNECTION_POOLING)" );
			let rc=::SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv); THROW_IF( rc==SQL_ERROR, "({}) - Unable to allocate an environment handle", rc );
			CALL(hEnv, SQL_HANDLE_ENV, ::SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3_80, 0), "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)" );
			return hEnv;
		}
		TRACE( "_hEnv={:x}", (uint)_hEnv );
		return _hEnv;
	}

	HandleSession::HandleSession()ε{
		BREAK;
	}
	HandleSession::HandleSession( sv connectionString )ε{
		let hEnv = getEnvHandle();
		CALL(hEnv, SQL_HANDLE_ENV, ::SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &_hStatement), "SQLAllocHandle");
		Connect( connectionString );
	}

	HandleSession::~HandleSession() {
		if( _hStatement ){
			::SQLDisconnect( _hStatement );
			::SQLFreeHandle( SQL_HANDLE_DBC, _hStatement );
		}
		if( _hEnv )
			::SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
	}

	α HandleSession::Connect( sv connectionString )ε->void{
		SQLCHAR connectionStringResult[8192];
		SQLSMALLINT connectionStringLength;
		CALL( _hStatement, SQL_HANDLE_DBC, ::SQLDriverConnect(_hStatement, nullptr, (SQLCHAR*)string(connectionString).c_str(), SQL_NTS, connectionStringResult, 8192, &connectionStringLength, SQL_DRIVER_NOPROMPT), "SQLDriverConnect" );
		if( connectionStringLength>0 )
			Logging::LogOnce( SRCE_CUR, _tags, "connectionString={}", (char*)connectionStringResult );
		else
			Critical( _tags, "connectionString Length={}", connectionStringLength );
	}

	HandleStatement::HandleStatement( string cs )ε:
		_session{ move(cs) }{
		CALL( _session, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, _session, &_hStatement), "SQLAllocHandle" );
	}
	HandleStatement::~HandleStatement(){
		if( _hStatement )	{
			if( let rc=SQLFreeHandle( SQL_HANDLE_STMT, _hStatement )!=SQL_SUCCESS )
				Warning( ELogTags::App, "SQLFreeHandle( SQL_HANDLE_STMT, {} ) returned {}", _hStatement, rc );
		}
	}
/*
	HandleStatementAsync::~HandleStatementAsync(){
		WARN_IF( _event && !::CloseHandle(_event), "CloseHandle returned {}", ::GetLastError() );
		let rc = _handle ? ::SQLFreeHandle( SQL_HANDLE_STMT, _handle ) : SQL_SUCCESS;
		WARN_IF( rc!=SQL_SUCCESS, "SQLFreeHandle(SQL_HANDLE_STMT) returned {} - {}", rc, ::GetLastError() );
	}

	auto HandleSessionAsync::Connect(sv connectionString)ε->void {
		if (IsAsynchronous()) {
			CALL(_hStatement, SQL_HANDLE_DBC, SQLSetConnectAttr(_hStatement, SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE, (SQLPOINTER)SQL_ASYNC_DBC_ENABLE_ON, SQL_IS_INTEGER), "SQLSetConnectAttr(SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE)");
			_event = CreateEvent(nullptr, false, false, nullptr); THROW_IF(!_event, "CreateEvent - {}"sv, GetLastError());
			CALL(_hStatement, SQL_HANDLE_DBC, SQLSetConnectAttr(_hStatement, SQL_ATTR_ASYNC_DBC_EVENT, _event, SQL_IS_POINTER), "SQLSetConnectAttr(SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE)");
		}
		else
			HandleSession::Connect(connectionString);
	}

	α HandleStatementAsync::OBindings()ε->const vector<up<IBindings>>&{
		if( !_bindings.size() ){
			SQLSMALLINT columnCount;
			CALL( _handle, SQL_HANDLE_STMT, SQLNumResultCols(_handle,&columnCount), "SQLNumResultCols" );
			if( columnCount ){
				_bindings.reserve( columnCount );
				let rowCount = RowStatusesSize(); //for( ; rowCount<RowStatusesSize() && _rowStatus[rowCount]==0 ; ++rowCount );
				for( SQLSMALLINT iCol = 1; iCol <= columnCount; ++iCol ){
					SQLLEN ssType;
					CALL( _handle, SQL_HANDLE_STMT, ::SQLColAttribute(_handle, iCol, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &ssType), "SQLColAttribute::Concise" );

					if( ssType == SQL_CHAR || ssType == SQL_VARCHAR || ssType == SQL_LONGVARCHAR || ssType == -9/*varchar(max)?* / ){
						SQLLEN bufferSize = 0;
						CALL( _handle, SQL_HANDLE_STMT, ::SQLColAttribute(_handle, iCol, SQL_DESC_DISPLAY_SIZE, NULL, 0, NULL, &bufferSize), "SQLColAttribute::Display" );
						if( ssType==-9 && bufferSize==0 )
							bufferSize = (1 << 14) - 1;//TODO handle varchar(max).
						_bindings.emplace_back( IBindings::Create((SQLSMALLINT)ssType, rowCount, ++bufferSize) );
					}
					else
						_bindings.emplace_back( IBindings::Create((SQLSMALLINT)ssType,rowCount) );
				}
			}
		}
		return _bindings;
	}
	*/
}