#include "AppQLAwait.h"
#include <jde/ql/QLAwait.h>
#include <jde/access/server/accessServer.h>
#include <jde/web/server/SettingQL.h>
#include "../LocalClient.h"
#include "ConnectionQLAwait.h"
#define let const auto

namespace Jde::App::Server{
	α AppQLAwait::Execute()ι->TAwait<jvalue>::Task{
		try{
			if( _queries.IsQueries() ){
				_raw = _raw && _queries.Queries().size()==1;
				jvalue y = _raw ? jvalue{} : jobject{};
				for( auto& q : _queries.Queries() ){
					q.ReturnRaw = true;
					let memberName = q.ReturnName();
					jvalue queryResult;
					if( q.JsonName.starts_with("connection") )
						queryResult = co_await ConnectionQLAwait{ move(q), UserPK(), _sl };
					else if( auto await = Access::Server::CustomQuery( q, UserPK(), _sl ); await )
						queryResult = co_await *await;
					else if( q.JsonName.starts_with( "setting" ) )
						queryResult = co_await Web::Server::SettingQLAwait{ q, AppClient(), _sl };
					else
						queryResult = co_await QL::QLAwait( move(q), UserPK(), _sl );
					if( _raw )
						y = queryResult;
					else
						y.get_object()[memberName] = move( queryResult );
				}
				Resume( move(y) );
			}
			else if( _queries.IsMutation() ){
				jarray results;
				for( auto& m : _queries.Mutations() ){
					m.ReturnRaw = _raw;
					using enum QL::EMutationQL;
					if( auto await = Access::Server::CustomMutation( m, UserPK(), _sl ); await )
						results.push_back( co_await *await );
					else
						results.push_back( co_await QL::QLAwait(move(m), UserPK(), _sl) );
				}
				jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
				Resume( move(y) );
			}
			else{
				Resume( co_await QL::QLAwait(move(_queries), UserPK(), _sl) );
			}
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}