#pragma once
#include "Exports.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include "OdbcAwaitables.h"
//#include "Binding.h"

extern "C" ΓODBC Jde::DB::IDataSource* GetDataSource();
namespace Jde::DB {
	struct IServerMeta;
	struct Sql;
	namespace Types { struct IRow; }
	namespace MsSql { struct MsSqlSchemaProc; }
}
namespace Jde::DB::Odbc{
	using namespace Coroutine;
	struct OdbcDataSource : IDataSource{
		α Disconnect()ε->void override;
		α ServerMeta()ι->IServerMeta& override;
		β AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		β AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;
		α Execute( string sql, SRCE )ε->uint override;
		α Execute( string sql, vec<Value>& params, SRCE)ε->uint override;
		α ExecuteCo( string sql, vector<Value> params, SRCE )ι->up<IAwait> override;
		α ExecuteCo( string sql, vector<Value> p, bool proc, RowΛ f, SRCE )ε->up<IAwait>;
		α Execute( string sql, const vector<Value>* params, const RowΛ* f, bool isStoredProc=false, SRCE )ε->uint override;
		α ExecuteProc( string sql, const vector<Value>& params, SRCE )ε->uint override;
		α ExecuteProc( string sql, const vector<Value>& params, RowΛ f, SRCE )ε->uint override;

		α Select( string sql, RowΛ f, const vector<Value>* pValues, SRCE )ε->uint override;
		α Select( Sql&& s, bool storedProc=false, SRCE )Ε->vector<up<IRow>> override;
		α SelectCo( ISelect* pAwait, string sql, vector<Value>&& params, SRCE )ι->up<IAwait> override;
		α SetConnectionString( string x )ι->void override;

		α ExecuteNoLog( string sql, const vector<Value>* params, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )ε->uint override;
		α ExecuteProcNoLog( string sql, vec<Value> params, SRCE )ε->uint override;
		α ExecuteProcCo( string sql, vector<Value> params, RowΛ f, SRCE )ε->up<IAwait> override;
		α ExecuteProcCo( string sql, vector<Value> params, SRCE )ι->up<IAwait> override;
		α SelectNoLog( string sql, RowΛ f, const vector<Value>* pValues, SRCE )ε->uint override;

		α Syntax()ι->const DB::Syntax& override{ return Syntax::Instance(); }
	private:
//		α Connect()ε{ return ConnectAwaitable{_connectionString/*, Asynchronous*/}; }
//		Ω Execute( HandleSessionAsync&& session, string&& sql, up<vector<up<Binding>>> pBindings, vector<Value> params, SL& sl )ι{ return ExecuteAwaitable( move(session), move(sql), move(pBindings), move(params), sl ); }
//		Ω Fetch( HandleStatementAsync&& h, ISelect* p )ι{ return FetchAwaitable( move(h), p ); }

		α ExecDirect( string sql, const RowΛ* f, const vector<Value>* pParams, SL sl, bool log = true )Ε->uint;
		///*SQLHDBC*/ α GetSession()ε->sp<void>;
		//α GetEnvironment()ε->sp<void>;
		up<MsSql::MsSqlSchemaProc> _schemaProc;
	};

/*	ⓣ OdbcDataSource::ScalerCo( sv sql, const vector<Value>&& parameters )ε->FunctionAwaitable//sp<T>
	{
		return FunctionAwaitable{ [sql,params=move(parameters),this]( coroutine_handle<Task2::promise_type> h )mutable->Task2
		{
			T result;
			RowΛ f = [&result](const IRow& row){ result = row.Get<T>(0); DBG("result={}"sv, result); };
			auto result2 = co_await SelectCo( sql, &f, &params, true );
			if( result2.HasError() )
				h.promise().get_return_object().SetResult( move(result2) );
			else
				h.promise().get_return_object().SetResult( make_shared<T>(result) );
			h.resume();
		}};
	}
	*/
}