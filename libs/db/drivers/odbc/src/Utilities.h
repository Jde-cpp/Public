#pragma once

namespace Jde::DB::Odbc{
	α HandleDiagnosticRecord( sv functionName, SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE retCode, SRCE )ε->string;
	Ξ Call( SQLHANDLE handle, SQLSMALLINT handleType, std::function<RETCODE()> func, sv functionName, SRCE )ε->void	{
		if( const RETCODE retCode = func(); retCode!=SQL_SUCCESS ){
			string diagnostics = HandleDiagnosticRecord( functionName, handle, handleType, retCode, sl );
			if( retCode==SQL_ERROR ) //was log-and-continue: callers ran on with uninitialized out-params (ssType/count/columnCount/…). SUCCESS_WITH_INFO(1)/NO_DATA(100) still pass.
				throw Exception{ sl, ELogLevel::Error, "({}) {} failed: {}", retCode, functionName, diagnostics };
		}
	}
	#define CALL( handle, handleType, function, functionName ) Call( handle, handleType, [&](){ return function; }, functionName )
}