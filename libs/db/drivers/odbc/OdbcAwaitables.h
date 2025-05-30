#pragma once

#include <jde/framework/coroutine/Task.h>
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/IDataSource.h>
#include "Binding.h"
#include "Handle.h"

namespace Jde::DB::Odbc
{
	using namespace Coroutine;
/*
	struct ConnectAwaitable final: IAwait{
		ConnectAwaitable( sv connectionString )ε: Session{}, ConnectionString{ connectionString }{}
		α await_ready()ι->bool;
		α await_suspend( std::coroutine_handle<> h )ι->void;
		α await_resume()ι->AwaitResult;
	private:
		sp<IException> ExceptionPtr;
//		HandleSessionAsync Session;
		sv ConnectionString;
	};
	*/
/*	struct ExecuteAwaitable final: IAwait
	{
		ExecuteAwaitable( HandleSessionAsync&& session, string&& sql, up<vector<up<Binding>>> pBindings, vector<Value> params, SL& sl )ι:IAwait{sl}, Statement{move(session)},_sql{move(sql)}, _pBindings{move(pBindings)}, _params{move(params)}{}
		α await_ready()ι->bool;
		α await_suspend( std::coroutine_handle<> h )ι->void;
		α await_resume()ι->AwaitResult;
	private:
		bool IsAsynchronous()Ι{ return Statement.IsAsynchronous(); }
		sp<IException> ExceptionPtr;
		string _sql;
		up<vector<up<Binding>>> _pBindings;
		vector<Value> _params;
		bool _log{true};
		//HandleStatementAsync Statement;
	};

	struct FetchAwaitable final : IAwait
	{
		FetchAwaitable( HandleStatementAsync&& statement, ISelect* p )ι:Statement{ move(statement) }, _function{p}{}
		α await_ready()ι->bool{return true;}
		α await_resume()ι->AwaitResult;
	private:
		bool IsAsynchronous()Ι{ return Statement.IsAsynchronous(); }
		sp<IException> ExceptionPtr;
		ISelect* _function;
		HandleStatementAsync Statement;
	};
	*/
}