#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/RequestQL.h>
#include <jde/db/generators/Statement.h>
#include "ops/MutationAwait.h"
#include "ops/TablesAwait.h"

namespace Jde::QL{
	struct TableQL;

	struct VQLAwait : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		VQLAwait( RequestQL&& request, optional<DB::Statement>&& statement, UserPK executer, SRCE )ι:base{sl}, _request{move(request)}, _statement{move(statement)}, _executer{executer}{}
	private:
		α Suspend()ι->void override;
		α Select( vector<TableQL>&& tables )ι->TablesAwait::Task;
		α Mutate( vector<MutationQL> mutations )ι->MutationAwait::Task;
		RequestQL _request;
		optional<DB::Statement>	_statement;
		UserPK _executer;
	};

	template<class T=jvalue>
	struct QLAwait : TAwaitEx<T, TAwait<jvalue>::Task>{
		using base = TAwaitEx<T, TAwait<jvalue>::Task>;
		QLAwait( TableQL&& ql, jobject variables, UserPK executer, SRCE )ι:base{sl},_request{move(ql), move(variables)}, _executer{executer}{}
		QLAwait( TableQL&& ql, jobject variables, DB::Statement&& statement, UserPK executer, SRCE )ι:
			base{sl}, _request{ move(ql), move(variables) }, _statement{ move(statement) }, _executer{ executer }{}
		QLAwait( MutationQL&& m, jobject variables, UserPK executer, SRCE )ι:
			base{sl}, _request{{move(m)}, move(variables)}, _executer{executer}{}
		QLAwait( string query, jobject variables, UserPK executer, const vector<sp<DB::AppSchema>>& schemas, bool returnRaw=true, SRCE )ε:
			base{sl}, _request{ Parse(move(query), variables, schemas, returnRaw) }, _executer{ executer }{}
	private:
		α Execute()ι->TAwait<jvalue>::Task override;
		RequestQL _request;
		optional<DB::Statement> _statement;
		UserPK _executer;
	};

	template<> Ξ QLAwait<jvalue>::Execute()ι->TAwait<jvalue>::Task{
		try{
			Resume( co_await VQLAwait{move(_request), move(_statement), _executer, base::_sl} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	template<> Ξ QLAwait<jobject>::Execute()ι->TAwait<jvalue>::Task{
		try{
			jvalue v = co_await VQLAwait{move(_request), move(_statement), _executer, base::_sl};
			Resume( v.is_null() ? jobject{} : move(Json::AsObject(v)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	template<> Ξ QLAwait<jarray>::Execute()ι->TAwait<jvalue>::Task{
		try{
			jvalue v = co_await VQLAwait{move(_request), move(_statement), _executer, base::_sl};
			Resume( move(Json::AsArray(v)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}