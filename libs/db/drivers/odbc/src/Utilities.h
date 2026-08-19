#pragma once
#include <jde/db/DBException.h>

namespace Jde::DB::Odbc{
	struct DiagnosticRecord{
		int32 Native{};
		EDbError Error{ EDbError::None };
	};

	α HandleDiagnosticRecord( sv functionName, SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE retCode, SRCE, DiagnosticRecord* pFirst=nullptr )ε->string;
	//The classified throw the statement paths share: the diagnostic queue supplies the message, SQLSTATE and native number, the caller the statement.
	[[noreturn]] α ThrowDiagnostic( sv functionName, SQLHANDLE hHandle, RETCODE retCode, Sql&& sql, SRCE )ε->void;
	Ξ Call( SQLHANDLE handle, SQLSMALLINT handleType, std::function<RETCODE()> func, sv functionName, SRCE )ε->void	{
		if( const RETCODE retCode = func(); retCode!=SQL_SUCCESS ){
			string diagnostics = HandleDiagnosticRecord( functionName, handle, handleType, retCode, sl );
			if( retCode==SQL_ERROR ) //was log-and-continue: callers ran on with uninitialized out-params (ssType/count/columnCount/…). SUCCESS_WITH_INFO(1)/NO_DATA(100) still pass.
				throw DBException{ retCode, Sql{}, Ƒ("{} failed: {}", functionName, diagnostics), sl };
		}
	}
	#define CALL( handle, handleType, function, functionName ) Call( handle, handleType, [&](){ return function; }, functionName )
}
