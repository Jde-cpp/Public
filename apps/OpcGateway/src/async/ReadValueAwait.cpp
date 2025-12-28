#include "ReadValueAwait.h"
#include <jde/fwk/process/execution.h>
#include <jde/opc/uatypes/Value.h>
#include "../UAClient.h"

#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)IotReadTag };
	Ω onResponse( UA_Client* /*ua*/, void* userdata, RequestId requestId, StatusCode sc, UA_DataValue* val )ι->void{
		ReadValueAwait& await = *( ReadValueAwait* )userdata;
		await.OnComplete( requestId, sc, val );
	}

	ReadValueAwait::ReadValueAwait( flat_set<NodeId> x, sp<UAClient> c, SL sl )ι:
		base{ sl },
		_nodes{ move(x) },
		_client{ move(c) }
	{}

	α ReadValueAwait::Suspend()ι->void{
		for( auto&& nodeId : _nodes ){
			RequestId requestId{};
			try{
				UAε( UA_Client_readValueAttribute_async(_client->UAPointer(), nodeId, onResponse, this, &requestId) );
				_requests.emplace( requestId, move(nodeId) );
				_client->Process( requestId, "readValueAttribute" );
			}
			catch( UAException& e ){
				_results.emplace( nodeId, Value{(StatusCode)e.Code} );
				if( _results.size()==_nodes.size() )
					Resume( move(_results) );
			}
		}
	}

	α ReadValueAwait::OnComplete( RequestId requestId, StatusCode sc, UA_DataValue* val )ι->void{
		_client->ClearRequest( requestId );
		auto logPrefix = [&](){ return Ƒ("[{}.{}]", hex(_client->Handle()), requestId); };
		auto nodeIdIt = _requests.find( requestId );
		if( nodeIdIt==_requests.end() ){
			CRITICAL( "{}ReadValueAwait::OnComplete - could not find requestId", logPrefix() );
			return;
		}
		Value value = sc || !val ? Value{ sc } : Value{ move(*val) };
		DBG( "{} Value: {}", logPrefix(), serialize(value.ToJson()) );
		_results.emplace( nodeIdIt->second, move(value) );
		if( _results.size()==_nodes.size() )
			Resume( move(_results) );
	}
}