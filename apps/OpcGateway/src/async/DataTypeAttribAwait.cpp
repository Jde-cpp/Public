#include "DataTypeAttribAwait.h"
#include "../UAClient.h"
#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	Ω onResponse( UA_Client*, void* userdata, RequestId requestId, StatusCode sc, UA_NodeId* dataType )ι->void{
		DataTypeAttribAwait& await = *(DataTypeAttribAwait*)userdata;
		await.OnComplete( requestId, sc, dataType );
	}

	α DataTypeAttribAwait::Suspend()ι->void{
		try{
			for( auto& nodeId : _nodeIds ){
				RequestId requestId{};
				UAε( UA_Client_readDataTypeAttribute_async(_client->UAPointer(), nodeId, onResponse, this, &requestId) );
				if( _requestNodes.empty() )
					_client->Process( requestId, "readDataTypeAttribute" );
				_requestNodes.emplace( requestId, move(nodeId) );
			}
		}
		catch( UAException& e ){
			ResumeExp( move(e) );
		}
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

		if( _results.size()==_requestNodes.size() ){
			_client->ClearRequest( _requestNodes.begin()->first );
			Resume( move(_results) );
		}
	}
}