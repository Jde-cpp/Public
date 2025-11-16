#include "GatewayQLAwait.h"
#include <jde/ql/QLAwait.h>
#include "../async/CallAwait.h"
#include "DataTypeQLAwait.h"
#include "NodeQLAwait.h"
#include "VariableQLAwait.h"
#define let const auto

namespace Jde::Opc::Gateway{
	GatewayQLAwait::GatewayQLAwait( QL::RequestQL&& q, sp<Web::Server::SessionInfo> session, bool returnRaw, SL sl )ι:
		base{sl},
		_raw{ returnRaw },
		_queries{ move(q) },
		_session{ move(session) }
	{}

	α GatewayQLAwait::Suspend()ι->void{
//		if( _queries.IsQueries() )
			Query();
//		else if( _queries.IsMutation() )
//			Mutate();
	}
	α GatewayQLAwait::Query()ι->TAwait<jvalue>::Task{
		try{
			if( _queries.IsQueries() ){
				jarray results;
				for( auto& q : _queries.Queries() ){
					q.ReturnRaw = _raw;
					if( q.JsonName.starts_with("serverConnection") || q.JsonName=="__type" ){
						if( q.JsonName=="__type" && !q.Args.contains("name") ){
							NodeId nodeId{ q.Args };
							q.Args["name"] = nodeId.ToString();
						}
						results.push_back( co_await QL::QLAwait<>(move(q), _session->UserPK, _sl) );
					}else if( q.JsonName.starts_with("node") )
						results.push_back( co_await NodeQLAwait{move(q), _session->SessionId, _session->UserPK, _sl} );
					else if( q.JsonName.starts_with("dataType") )
						results.push_back( co_await DataTypeQLAwait{move(q), _session, _sl} );
					else
						throw Exception{ _sl, "Unknown query type: {}", q.JsonName };
				}
				jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
				Resume( _raw ? move(y) : jobject{{"data", y}} );
			}
			else if( _queries.IsMutation() ){
				jarray results;
				for( auto& m : _queries.Mutations() ){
					m.ReturnRaw = _raw;
					// if( m.Type==QL::EMutationQL::Execute )
					// 	results.push_back( co_await JCallAwait(move(m), _request.SessionInfo, _sl) );
					if( m.DBTable && m.TableName()=="server_connections" )
						results.push_back( co_await QL::QLAwait<>(move(m), _session->UserPK, _sl) );
					else if( m.JsonTableName=="variable" )
						results.push_back( co_await VariableQLAwait{move(m), _session, _sl} );
				}
				jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
				Resume( _raw ? move(y) : jobject{{"data", move(y)}} );
			}
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
				if( m.Type==QL::EMutationQL::Execute )
					results.push_back( co_await JCallAwait(move(m), _session, _sl) );
				else if( m.TableName()=="server_connections" )
					results.push_back( co_await QL::QLAwait<>(move(m), _session->UserPK, _sl) );
			}
			jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
			Resume( _raw ? move(y) : jobject{{"data", move(y)}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}