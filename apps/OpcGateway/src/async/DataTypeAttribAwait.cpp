#include "DataTypeAttribAwait.h"
#include "../UAClient.h" //!important
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	Ω onResponse( UA_Client*, void* userdata, RequestId requestId, StatusCode sc, UA_NodeId* dataType )ι->void{
		DataTypeAttribAwait& await = *(DataTypeAttribAwait*)userdata;
		await.OnComplete( requestId, sc, dataType );
	}

	//Suspend's closure and OnComplete (invoked inside run_iterate) both run on the client's strand, so
	//_requestNodes/_results are strand-confined - no lock, and a response can't arrive before its requestId
	//is registered (run_iterate is queued behind this closure).
	α DataTypeAttribAwait::Suspend()ι->void{
		_client->PostUA( [this]{
			try{
				for( auto& nodeId : _nodeIds ){
					RequestId requestId{};
					UAε( UA_Client_readDataTypeAttribute_async(_client->UAPointer(), nodeId, onResponse, this, &requestId) );
					_requestNodes.emplace( requestId, move(nodeId) );
					if( _requestNodes.size()==1 )
						_client->Process( requestId, "readDataTypeAttribute" );
				}
			}
			catch( UAException& e ){
				ResumeExp( move(e) );
			}
		});
	}
	α DataTypeAttribAwait::OnComplete( RequestId requestId, StatusCode sc, UA_NodeId* dataType )ι->void{
		auto p = _requestNodes.find( requestId );
		RETURN_IF( p==_requestNodes.end(), ELogLevel::Critical, "[{}.{}]Could not find requestId in requestNodes.", hex(_client->Handle()), hex(requestId) );
		if( sc ){
			DBG( "[{}.{}]DataTypeAttribAwait - {}", hex(_client->Handle()), hex(requestId), UAException::Message(sc) );
			_results.try_emplace( move(p->second), sc );
		}
		else{
			TRACE( "[{}.{}]DataTypeAttribAwait - dataType: {}", hex(_client->Handle()), hex(requestId), NodeId{*dataType}.ToString() );
			_results.try_emplace( move(p->second), NodeId{move(*dataType)} );
		}
		if( _results.size()==_nodeIds.size() ){
			_client->ClearRequest( _requestNodes.begin()->first );
			Resume( move(_results) );
		}
	}
}