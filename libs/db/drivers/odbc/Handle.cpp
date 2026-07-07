#include "Handle.h"
#include <sql.h>
#include <sqlext.h>
#include <jde/db/DBException.h>
#include "Utilities.h"

#define let const auto
namespace Jde::DB::Odbc{
	constexpr ELogTags _tags{ ELogTags::App };
	SQLHENV _hEnv{}; //shared by all sessions so SQL_CP_ONE_PER_HENV pooling works; lives for the process.
	mutex _hEnvMutex;

	Ω getEnvHandle()ε->SQLHENV{
		lg l{ _hEnvMutex };
		if( !_hEnv ){
			CALL( nullptr, SQL_HANDLE_ENV, ::SQLSetEnvAttr(nullptr, SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER)SQL_CP_ONE_PER_HENV, 0), "SQLSetEnvAttr(SQL_ATTR_CONNECTION_POOLING)" );
			SQLHENV hEnv;
			let rc=::SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv); THROW_IF( rc==SQL_ERROR, "({}) - Unable to allocate an environment handle", rc );
			CALL(hEnv, SQL_HANDLE_ENV, ::SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3_80, 0), "SQLSetEnvAttr(SQL_ATTR_ODBC_VERSION)" );
			_hEnv = hEnv;
		}
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
	}

	α HandleSession::Connect( sv connectionString )ε->void{
		SQLCHAR connectionStringResult[8192];
		SQLSMALLINT connectionStringLength;
		CALL( _hStatement, SQL_HANDLE_DBC, ::SQLDriverConnect(_hStatement, nullptr, (SQLCHAR*)string(connectionString).c_str(), SQL_NTS, connectionStringResult, 8192, &connectionStringLength, SQL_DRIVER_NOPROMPT), "SQLDriverConnect" );
		if( connectionStringLength>0 )
			Logging::LogOnce( SRCE_CUR, _tags, "connectionString={}", (char*)connectionStringResult );
		else
			CRITICAL( "connectionString Length={}", connectionStringLength );
	}

	HandleStatement::HandleStatement( string cs )ε:
		_session{ move(cs) }{
		CALL( _session, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, _session, &_hStatement), "SQLAllocHandle" );
	}
	HandleStatement::~HandleStatement(){
		if( _hStatement )	{
			if( let rc=SQLFreeHandle( SQL_HANDLE_STMT, _hStatement )!=SQL_SUCCESS )
				WARNT( ELogTags::App, "SQLFreeHandle( SQL_HANDLE_STMT, {} ) returned {}", _hStatement, rc );
		}
	}
}