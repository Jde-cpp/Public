﻿#include <jde/iot/uatypes/Browse.h>
#include <jde/iot/uatypes/UAClient.h>
#include <jde/iot/uatypes/Value.h>
#include "../async/Attributes.h"
#include <jde/iot/async/SessionAwait.h>

namespace Jde::Iot::Browse{
	α FoldersAwait::await_suspend( HCoroutine h )ι->void{
		IAwait::await_suspend( h );
		_client->SendBrowseRequest( Request{move(_node)}, move(h) );
	}

	α Folders( NodeId node, sp<UAClient>& c )ι->FoldersAwait{ return FoldersAwait{ move(node), c }; }
	ObjectsFolderAwait::ObjectsFolderAwait( NodeId node, bool snapshot, sp<UAClient> ua, SL sl )ι:
		base{ sl },
		_ua{ua},
		_node{node},
		_snapshot{snapshot}
	{}


	α ObjectsFolderAwait::Execute()ι->Coroutine::Task{
		bool retry{};
		try{
			auto y = ( co_await Folders(_node, _ua) ).SP<Response>();
			flat_set<NodeId> nodes = y->Nodes();
			THROW_IF( nodes.size()==0, "No items found for: {}", _node.to_string() );

			up<flat_map<NodeId, Value>> pValues;
			if( _snapshot )
			 	pValues = ( co_await Read::SendRequest(nodes, _ua) ).UP<flat_map<NodeId, Value>>();
			auto pDataTypes = (co_await Attributes::ReadDataTypeAttributes( move(nodes), _ua )).UP<flat_map<NodeId, NodeId>>();
			Promise()->SetValue( y->ToJson(move(pValues), move(*pDataTypes)) );
		}
		catch( UAException& e ){
			if( retry=e.IsBadSession(); retry )
				e.PrependWhat( "Retry ObjectsFolder.  " );
			else
				Promise()->SetError( move(e) );
		}
		catch( IException& e ){
			Promise()->SetError( move(e) );
		}
		if( retry ){
			try{ co_await AwaitSessionActivation( _ua ); }catch(IException& e){ Promise()->SetError( move(e) ); }
			[this]()->Task {
				try{
					auto j = co_await ObjectsFolderAwait{ _node, _snapshot, _ua, _sl };
					Promise()->SetValue( move(j) );
				}
				catch( IException& e ){
					Promise()->SetError( move(e) );
				}
			}();
			_h.resume();
		}
	}
	α ObjectsFolderAwait::await_suspend( base::Handle h )ι->void{
		base::await_suspend( h );
		Execute();
	}

	α OnResponse( UA_Client *ua, void* userdata, RequestId requestId, UA_BrowseResponse* response )ι->void{
		auto h = UAClient::ClearRequestH( ua, requestId ); if( !h ){ Critical( BrowseTag, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId ); return; }
		if( !response->responseHeader.serviceResult )
			Resume( ms<Response>(move(*response)), move(h) );
		else
			Resume( UAException{response->responseHeader.serviceResult, ua, requestId}, move(h) );
	}

	Request::Request( NodeId&& node )ι{
		UA_BrowseRequest_init( this );
    requestedMaxReferencesPerNode = 0;
    nodesToBrowse = UA_BrowseDescription_new();
    nodesToBrowseSize = 1;
    nodesToBrowse[0].nodeId = node.Move();
    nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
	}
	α Response::Nodes()ι->flat_set<NodeId>{
		flat_set<NodeId> y;
		for( uint i = 0; i < resultsSize; ++i) {
      for( size_t j = 0; j < results[i].referencesSize; ++j )
				y.emplace( results[i].references[j].nodeId );
		}
		return y;
	}
	α Response::ToJson( up<flat_map<NodeId, Value>>&& pSnapshot, flat_map<NodeId, NodeId>&& dataTypes )ε->json{
		try{
			json references = json::array();
	    for(size_t i = 0; i < resultsSize; ++i) {
	      for(size_t j = 0; j < results[i].referencesSize; ++j) {
	        UA_ReferenceDescription& ref = results[i].references[j];
					const NodeId nodeId{ move(ref.nodeId) };
					json reference;
					if( pSnapshot ){
						if( auto p = pSnapshot->find(nodeId); p!=pSnapshot->end() )
							reference["value"] = p->second.ToJson();
					}
					if( auto p = dataTypes.find(nodeId); p!=dataTypes.end() )
						reference["dataType"] = Iot::ToJson( p->second );
					else
						Warning( BrowseTag, "Could not find data type for node={}.", nodeId.ToJson().dump() );
					reference["referenceType"] = Iot::ToJson( ref.referenceTypeId );
					reference["isForward"] = ref.isForward;
					reference["node"] = Iot::ToJson( nodeId );

					json bn;
					const UA_QualifiedName& browseName = ref.browseName;
					bn["ns"] = browseName.namespaceIndex;
					bn["name"] = ToSV( browseName.name );
					reference["browseName"] = bn;

					json dn;
					const UA_LocalizedText& displayName = ref.displayName;
					dn["locale"] = ToSV( displayName.locale );
					dn["text"] = ToSV( displayName.text );
					reference["displayName"] = dn;

					reference["nodeClass"] = ref.nodeClass;
					reference["typeDefinition"] = Iot::ToJson( ref.typeDefinition );

					references.push_back( reference );
				}
			}
			json j;
			j["references"] = references;
			return j;
		}
		catch( json::exception& e ){
			throw Exception{ SRCE_CUR, move(e), ELogLevel::Critical };
		}
	}
}