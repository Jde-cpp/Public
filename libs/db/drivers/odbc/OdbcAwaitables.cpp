#include "OdbcAwaitables.h"
#include "OdbcWorker.h"
#include <jde/db/DBException.h>
#include "Utilities.h"
#include "OdbcRow.h"

#define var const auto
namespace Jde::DB::Odbc{
/*
	α ConnectAwaitable::await_ready()ι->bool{
		try{
			Session.Connect( ConnectionString );
		}
		catch( DBException& e ){
			ExceptionPtr = e.Move();
		}
		catch( ... ){
			Critical( ELogTags::App, "Unexpected Exception" );
		}
		return true;//ExceptionPtr!=nullptr;
	}
	α ConnectAwaitable::await_suspend( std::coroutine_handle<> h )ι->void{
		ASSERT(false);
		CoroutinePool::Resume( move(h) );
	}
	α ConnectAwaitable::await_resume()ι->AwaitResult{
		return ExceptionPtr ? AwaitResult{ ExceptionPtr->Move() } : AwaitResult{ ms<HandleSessionAsync>(move(Session)) };
	}

	α ExecuteAwaitable::await_ready()ι->bool{
		try{
			SQLHSTMT h;
			CALL( Statement.Session(), SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, Statement.Session(), &h), "SQLAllocHandle" );
			Statement.SetHandle( h );
			CALL( Statement.Session(), SQL_HANDLE_DBC, SQLSetStmtAttr(h, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)Statement.RowStatusesSize(), 0), "SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE)" );
			CALL( Statement.Session(), SQL_HANDLE_DBC, SQLSetStmtAttr(h, SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)Statement.RowStatuses(), 0), "SQLSetStmtAttr(SQL_ATTR_ROW_STATUS_PTR)" );
			if( _pBindings ){
				SQLUSMALLINT i = 0;
				for( var& pBinding : *_pBindings )
				{
					var result = ::SQLBindParameter( Statement, ++i, SQL_PARAM_INPUT, pBinding->CodeType, pBinding->DBType, pBinding->Size(), pBinding->DecimalDigits(),  pBinding->Data(), pBinding->BufferLength(), &pBinding->Output );  THROW_IFX( result<0, DBException(result, _sql, nullptr, format("parameter {} returned {} - {}", i-1, result, ::GetLastError()), SRCE_CUR) );
					THROW_IFX( !SUCCEEDED(result), DBException(move(_sql), nullptr, format("SQLBindParameter {}", result)) );
				}
			}
			BREAK_IF( _sql.find("call")!=string::npos );
			var retCode = ::SQLExecDirect( Statement, (SQLCHAR*)_sql.data(), (SQLINTEGER)_sql.size() );
			if( SUCCEEDED(retCode) ){
				HandleDiagnosticRecord( "SQLExecDirect", Statement, SQL_HANDLE_STMT, retCode );
				SQLLEN count;
				CALL( Statement, SQL_HANDLE_STMT, ::SQLRowCount(Statement,&count), "SQLRowCount" );
				Statement._result = count;
			}
			else if( retCode==SQL_INVALID_HANDLE )
				throw DBException( retCode, move(_sql), &_params, "SQL_INVALID_HANDLE" );
			else if( retCode==SQL_ERROR )
				throw DBException{ retCode, move(_sql), &_params, HandleDiagnosticRecord("SQLExecDirect", Statement, SQL_HANDLE_STMT, retCode, _sl), _sl };
			else
				throw DBException( retCode, move(_sql), &_params, "Unknown error" );
		}
		catch( DBException& e ){
			ExceptionPtr = e.Move();
		}
		return true;
	}
	α ExecuteAwaitable::await_suspend( std::coroutine_handle<> h )ι->void{
		ASSERT(false);
		OdbcWorker::Push( move(h), Statement.Event(), false );//never gets called.
	}
	α ExecuteAwaitable::await_resume()ι->AwaitResult{
		return ExceptionPtr ? AwaitResult{ ExceptionPtr } : AwaitResult{ ms<HandleStatementAsync>(move(Statement)) };
	}

/*	α FetchAwaitable::await_ready()ι->bool
	{
		return !IsAsynchronous();
		//try
		//{
		//	var hr = ::SQLFetch( Statement );
		//	THROW_IF( !SQL_SUCCEEDED(hr), "SQLFetch returned {} - {}", hr, GetLastError() );
		//}
		//catch( const Exception& e )
		//{
		//	ExceptionPtr = std::make_exception_ptr( e );
		//}
		//return !IsAsynchronous() || ExceptionPtr!=nullptr;
	}
	α FetchAwaitable::await_suspend( std::coroutine_handle<> h )ι->void
	{
		OdbcWorker::Push( move(h), Statement.Event(), false );
	}* /
	α FetchAwaitable::await_resume()ι->AwaitResult{
		try{
			var& bindings = Statement.OBindings();

/*				BindingInt32s ib{5};
			//SQLLEN ids[50]={0};
			CALL( Statement, SQL_HANDLE_STMT, ::SQLBindCol(Statement, 1, (SQLSMALLINT)ib.CodeType(), ib.Data(), ib.BufferLength(), ib.OutputPtr()), "SQLBindCol" );
			BindingStrings<SQL_CHAR> sb{5, 256};
			SQLLEN l = sb.BufferLength();
			SQLLEN x[50]={0};
			char sz[256][256];
			//DBG( "sizeof={:x} data={:x}, DBType={:x}, CodeType={:x} Output={:x} Buffer={:x} size={:x}", sizeof(BindingString), (uint16)&sb, (uint16)&sb.DBType, (uint16)&sb.CodeType, (uint16)&sb.Output, (uint16)&sb._pBuffer, (uint16)&sb._size );
			var hr2 = ::SQLBindCol( Statement, 2, (SQLSMALLINT)sb.CodeType(), sz, 10, x );//sb.BufferLength()
			//CALL( Statement, SQL_HANDLE_STMT, ::SQLBindCol(Statement, 2, (SQLSMALLINT)sb.CodeType, sb.Data(), sb.BufferLength(), &sb.Output), "SQLBindCol" );
			var hr3 = ::SQLFetch( Statement );
			* /
			SQLUSMALLINT i=0;
			for( var& p : bindings )
				CALL( Statement, SQL_HANDLE_STMT, ::SQLBindCol(Statement, ++i, (SQLSMALLINT)p->CodeType(), p->Data(), p->BufferLength(), p->OutputPtr()), "SQLBindCol" );

			OdbcRowMulti row( bindings );
			HRESULT hr;
			while( (hr = ::SQLFetch(Statement))==SQL_SUCCESS || hr==SQL_SUCCESS_WITH_INFO ){
				uint i=0;
				for( ; i<Statement.RowStatusesSize() && Statement.RowStatuses()[i]==SQL_ROW_SUCCESS; ++i ){
					_function->OnRow( row );
					row.Reset();
				}
				if( i<Statement.RowStatusesSize() && Statement.RowStatuses()[i]==SQL_ROW_NOROW )
					break;
				ASSERT( i==Statement.RowStatusesSize() || Statement.RowStatuses()[i]!=SQL_ROW_ERROR );//allocate more space.
				row.ResetRowIndex();
			}
			THROW_IF( hr!=SQL_NO_DATA && !SQL_SUCCEEDED(hr), "SQLFetch returned {} - {}", hr, GetLastError() );
		}
		catch( IException& e ){
			ExceptionPtr = e.Move();
		}
		return ExceptionPtr ? AwaitResult{ ExceptionPtr } : AwaitResult{ ms<HandleStatementAsync>(move(Statement)) };
	}
	*/
}