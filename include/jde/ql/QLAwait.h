#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/RequestQL.h>
#include <jde/db/generators/Statement.h>
#include "ops/MutationAwait.h"
#include "ops/TablesAwait.h"

//namespace Jde::DB{ struct Statement; }
namespace Jde::QL{
	struct TableQL;
	template<class T=jobject>
	α QuerySync( string query, UserPK executer, bool returnRaw=true, SRCE )ε->T;

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
	struct QLAwait : TAwaitEx<T, VQLAwait::Task>{
		using base = TAwaitEx<T, VQLAwait::Task>;
		QLAwait( TableQL&& ql, UserPK executer, SRCE )ι:base{sl},_request{move(ql)}, _executer{executer}{}
		QLAwait( TableQL&& ql, DB::Statement&& statement, UserPK executer, SRCE )ι:
			base{sl}, _request{ move(ql) }, _statement{ move(statement) }, _executer{ executer }{}
		QLAwait( MutationQL&& m, UserPK executer, SRCE )ι:
			base{sl}, _request{{move(m)}}, _executer{executer}{}
		QLAwait( string query, UserPK executer, bool returnRaw=true, SRCE )ε:
			base{sl}, _request{ Parse(move(query), returnRaw) }, _executer{ executer }{}
	private:
		α Execute()ι->VQLAwait::Task override;
		RequestQL _request;
		optional<DB::Statement> _statement;
		UserPK _executer;
	};

	template<> Ξ QLAwait<jvalue>::Execute()ι->VQLAwait::Task{
		try{
			Resume( co_await VQLAwait{move(_request), move(_statement), _executer, base::_sl} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	template<> Ξ QLAwait<jobject>::Execute()ι->VQLAwait::Task{
		try{
			jvalue v = co_await VQLAwait{move(_request), move(_statement), _executer, base::_sl};
			Resume( v.is_null() ? jobject{} : move(Json::AsObject(v)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	template<> Ξ QLAwait<jarray>::Execute()ι->VQLAwait::Task{
		try{
			jvalue v = co_await VQLAwait{move(_request), move(_statement), _executer, base::_sl};
			Resume( move(Json::AsArray(v)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}
namespace Jde{
		Ŧ QL::QuerySync( string query, UserPK executer, bool returnRaw, SL sl )ε->T{
		using Await=QL::QLAwait<T>;
		Await await{ move(query), executer, returnRaw, sl };
		return BlockAwait<Await,T>( await );
	}
}