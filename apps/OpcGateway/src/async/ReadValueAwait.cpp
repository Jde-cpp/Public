#include "ReadValueAwait.h"
#include <jde/fwk/process/execution.h>
#include <jde/opc/uatypes/Value.h>
#include "../UAClient.h"

#define let const auto

namespace Jde::Opc::Gateway{
		ReadValueAwait::ReadValueAwait( flat_set<NodeId> x, sp<UAClient> c, SL sl )ι:
			TAwait<flat_map<NodeId, Value>>{sl},
			_nodes{move(x)},
			_client{move(c)}
		{}

		α ReadValueAwait::Suspend()ι->void{
			_client->SendReadRequest( move(_nodes), _h );
		}
namespace Read{
	α OnResponse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode sc, UA_DataValue* val )ι->void{
		let handle = userdata ? (RequestId)(uint)userdata : requestId;
		string logPrefix = format( "[{:x}.{}.{}]", (uint)ua, handle, requestId );
		if( sc )
			TRACET( IotReadTag, "{}Value::OnResponse ({})-{} Value={}", logPrefix, sc, UAException::Message(sc), val ? serialize(Value{*val}.ToJson()) : "null" );
		auto pClient = UAClient::TryFind(ua); if( !pClient ) return;
		up<flat_map<NodeId, Value>> results;
		bool visited = pClient->_readRequests.visit( handle, [requestId, sc, val, &results, &logPrefix]( auto& pair ){
			auto& x = pair.second;
			if( auto pRequest=x.Requests.find(requestId); pRequest!=x.Requests.end() ){
				auto p = x.Results.try_emplace( pRequest->second, sc ? Value{ sc } : Value{ move(*val) } ).first;
				TRACET( IotReadTag, "{} Value={}", logPrefix, sc ? format("[{:x}]{}", sc, UAException::Message(sc)) : serialize(p->second.ToJson()) );
				if( x.Results.size()==x.Requests.size() )
					results = mu<flat_map<NodeId, Value>>( move(x.Results) );
			}
		});
		if( !visited ){
			CRITICALT( IotReadTag,  "{}Could not find handle.", logPrefix );
		}else if( results ){
			pClient->_readRequests.erase( handle );
			auto h = pClient->ClearRequestH<ReadValueAwait::Handle>( handle ); if( !h ){ CRITICALT(IotReadTag, "[{}]Could not find handle.", logPrefix); return; };
			TRACET( IotReadTag, "{}Resume", logPrefix );
			Post<flat_map<NodeId, Value>>( move(*results), move(h) );
		}
	}
}}