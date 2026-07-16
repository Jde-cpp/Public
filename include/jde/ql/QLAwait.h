#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/types/RequestQL.h>
#include <jde/db/generators/Statement.h>
#include "ops/MutationAwait.h"
#include "ops/TablesAwait.h"

namespace Jde::QL{
	struct TableQL;

	template<class T=jvalue>
	struct QLAwait : TAwaitEx<T, TAwait<jvalue>::Task>, noncopyable{
		using base = TAwaitEx<T, TAwait<jvalue>::Task>;
		QLAwait( TableQL&& ql, UserPK executer, SRCE )ι:base{sl},_request{move(ql)}, _executer{executer}{}
		QLAwait( TableQL&& ql, optional<DB::Statement> statement, UserPK executer, SRCE )ι:
			base{sl}, _request{ move(ql) }, _statement{ move(statement) }, _executer{ executer }{
			ASSERT( !_statement || _statement->From.Joins.size() );
		}
		QLAwait( MutationQL&& m, UserPK executer, SRCE )ι:
			base{sl}, _request{{move(m)}}, _executer{executer}{}
		QLAwait( string query, jobject variables, UserPK executer, sp<IQL> ql, bool returnRaw=true, SRCE )ε:
			base{sl}, _request{ Parse(move(query), move(variables), ql->Schemas(), returnRaw) }, _executer{ executer }, _ql{ql}{}
		QLAwait( RequestQL&& q, QL::Creds executer, sp<IQL> ql, SRCE )ε:base{sl}, _request{move(q)}, _executer{executer}, _ql{ql}{}
		QLAwait( RequestQL&& q, optional<DB::Statement>&& statement, QL::Creds creds, sp<IQL> ql, SRCE )ι://engine ctor: dispatch a request that already carries its statement (used by the jobject/jarray adapters).
			base{sl}, _request{move(q)}, _statement{move(statement)}, _executer{move(creds)}, _ql{move(ql)}{}

	private:
		α Execute()ι->TAwait<jvalue>::Task override;
		RequestQL _request;
		optional<DB::Statement> _statement;
		QL::Creds _executer;
		sp<IQL> _ql;
	};

	template<> α QLAwait<jvalue>::Execute()ι->TAwait<jvalue>::Task;//the engine: dispatches _request to Tables/Mutation awaits (defined in QLAwait.cpp). Must be declared before the adapters below co_await a QLAwait<jvalue>.
	template<> Ξ QLAwait<jobject>::Execute()ι->TAwait<jvalue>::Task{
		try{
			jvalue v = co_await QLAwait<jvalue>{move(_request), move(_statement), _executer, _ql, base::_sl};
			Resume( v.is_null() ? jobject{} : move(Json::AsObject(v)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	template<> Ξ QLAwait<jarray>::Execute()ι->TAwait<jvalue>::Task{
		try{
			jvalue v = co_await QLAwait<jvalue>{move(_request), move(_statement), _executer, _ql, base::_sl};
			auto a = Json::AsArray(v);
			Resume( move(a) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}