#include <jde/iot/uatypes/Browse.h>
#include <jde/iot/uatypes/UAClient.h>
#include <jde/iot/uatypes/Value.h>
#include "../async/Attributes.h"
#include <jde/iot/async/SessionAwait.h>

namespace Jde::Iot::Browse{
	sp<Jde::LogTag> _logTag{ Logging::Tag("app.browse") };
	α Tag()ι->sp<LogTag>{return _logTag;}

	α FoldersAwait::await_suspend( HCoroutine h )ι->void{
		IAwait::await_suspend( h );
		_client->SendBrowseRequest( Request{move(_node)}, move(h) );
	}

	α Folders( NodeId node, sp<UAClient>& c )ι->FoldersAwait{ return FoldersAwait{ move(node), c }; }

	α ObjectsFolder( sp<UAClient> ua, NodeId node, Web::Rest::Request req, bool snapshot )ι->Task{
		bool retry{};
		try{
			auto y = ( co_await Folders(node, ua) ).SP<Response>();
			flat_set<NodeId> nodes = y->Nodes();
			THROW_IF( nodes.size()==0, "No items found for: {}", node.to_string() );

			up<flat_map<NodeId, Value>> pValues;

			if( snapshot )
			 	pValues = ( co_await Read::SendRequest(nodes, ua) ).UP<flat_map<NodeId, Value>>();
			auto pDataTypes = (co_await Attributes::ReadDataTypeAttributes( move(nodes), move(ua) )).UP<flat_map<NodeId, NodeId>>();
			Web::Rest::ISession::Send(y->ToJson(move(pValues), move(*pDataTypes)), move(req) );
		}
		catch( UAException& e ){
			if( retry=e.IsBadSession(); retry )
				e.PrependWhat( "Retry ObjectsFolder.  " );
			else{
				TRACE( "ObjectsFolder - Failed {}", e.what() );
				Web::Rest::ISession::Send( move(e), move(req) );
			}
		}
		catch( IException& e ){
			Web::Rest::ISession::Send( move(e), move(req) );
		}
		if( retry ){
			co_await AwaitSessionActivation( ua );
			ObjectsFolder( ua, node, move(req), snapshot );
		}
	}

	α OnResponse( UA_Client *ua, void* userdata, RequestId requestId, UA_BrowseResponse* response )ι->void{
		auto h = UAClient::ClearRequestH( ua, requestId ); RETURN_IF( !h, ELogLevel::Critical, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId );

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
						WARN( "Could not find data type for node={}.", nodeId.ToJson().dump() );
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
		catch( json::exception& e )
		{
			throw Exception{ SRCE_CUR, move(e), ELogLevel::Critical };
		}
	}
}