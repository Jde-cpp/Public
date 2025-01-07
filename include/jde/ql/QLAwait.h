#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/RequestQL.h>
#include <jde/db/generators/Statement.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"


//namespace Jde::DB{ struct Statement; }
namespace Jde::QL{
	struct TableQL;
	α Query( const TableQL& ql, DB::Statement&& statement, UserPK executer, SRCE )ε->jvalue;
	α Query( RequestQL&& ql, UserPK executer, SL sl )ε->jvalue;

/*	struct IQLAwait{
		IQLAwait( TableQL&& ql, UserPK executer, SRCE )ι:_request{move(ql)}, _executer{executer}{}
		IQLAwait( TableQL&& ql, DB::Statement&& statement, UserPK executer, SRCE )ι;
		IQLAwait( string query, UserPK executer, SRCE )ε;
	private:
		RequestQL _request;
		optional<DB::Statement> _statement;
		UserPK _executer;
	};*/
	template<class T=jvalue>
	struct QLAwait : TAwait<T>{
		using base = TAwait<T>;
		QLAwait( TableQL&& ql, UserPK executer, SRCE )ι:_request{move(ql)}, _executer{executer}{}
		QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK executer, SRCE )ι:
		base{sl}, _request{ move(ql) }, _statement{ move(statement) }, _executer{ executer }{}
		QLAwait( string query, UserPK executer, SRCE )ε:
		base{sl}, _request{ Parse(move(query)) }, _executer{ executer }{}

		α Suspend()ι->void override{ CoroutinePool::Resume( base::_h ); }
		α Run()ε->jvalue;
		α await_resume()ε->T override;
	private:
		RequestQL _request;
		optional<DB::Statement> _statement;
		UserPK _executer;
	};

	Ŧ QLAwait<T>::Run()ε->jvalue{
		jvalue y;
		if( _statement )
			y = Query( _request.TableQLs().front(), move(*_statement), _executer, base::_sl );
		else
			y = Query( move(_request), _executer, base::_sl );
		return y;
	}
	template<> Ξ QLAwait<jvalue>::await_resume()ε->jvalue{ return Run(); }
	template<> Ξ QLAwait<jobject>::await_resume()ε->jobject{ return Json::AsObject(Run(), _sl); }
	template<> Ξ QLAwait<jarray>::await_resume()ε->jarray{ return Json::AsArray(Run(), _sl); }
}