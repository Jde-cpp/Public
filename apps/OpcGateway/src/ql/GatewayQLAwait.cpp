#include "GatewayQLAwait.h"
#include <jde/ql/QLAwait.h>
#include "NodeQLAwait.h"
#define let const auto

namespace Jde::Opc::Gateway{
	GatewayQLAwait::GatewayQLAwait( Web::Server::HttpRequest&& request, QL::RequestQL&& q, SL sl )ι:
		base{sl},
		_raw{ request.Params().contains("raw") },
		_request{ move(request) },
		_queries{ move(q) }
	{}

	α GatewayQLAwait::Suspend()ι->void{
		if( _queries.IsQueries() )
			Query();
		else if( _queries.IsMutation() )
			Mutate();
	}
	α GatewayQLAwait::Query()ι->TAwait<jvalue>::Task{
		try{
			jarray results;
			for( auto& q : _queries.Queries() ){
				q.ReturnRaw = _raw;
				if( q.JsonName.starts_with("serverConnection") )
					results.push_back( co_await QL::QLAwait<>(move(q), _request.SessionInfo->UserPK, _sl) );
				if( q.JsonName.starts_with("node") )
					results.push_back( co_await NodeQLAwait{move(q), _request.SessionInfo->SessionId, _request.SessionInfo->UserPK, _sl} );
			}
			jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
			Resume( Web::Server::HttpTaskResult{_raw ? y : jobject{{"data", y}}, move(_request)} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α GatewayQLAwait::Mutate()ι->TAwait<jvalue>::Task{
		try{
			jarray results;
			for( auto& m : _queries.Mutations() ){
				m.ReturnRaw = _raw;
				if( m.TableName()=="server_connections" )
					results.push_back( co_await QL::QLAwait<>(move(m), _request.SessionInfo->UserPK, _sl) );
			}
			jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
			Resume( Web::Server::HttpTaskResult{_raw ? y : jobject{{"data", y}}, move(_request)} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}