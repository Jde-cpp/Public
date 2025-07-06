#include <jde/opc/uatypes/Browse.h>
#include <jde/opc/uatypes/UAClient.h>
#include <jde/opc/uatypes/Value.h>
#include "../async/Attributes.h"
#include <jde/opc/async/SessionAwait.h>

namespace Jde::Opc::Browse{
	α FoldersAwait::Suspend()ι->void{
		_client->SendBrowseRequest( Request{move(_node)}, _h );
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
			Resume( y->ToJson(move(pValues), move(*pDataTypes)) );
		}
		catch( Client::UAClientException& e ){
			if( retry=e.IsBadSession(); retry )
				e.PrependWhat( "Retry ObjectsFolder.  " );
			else
				ResumeExp( move(e) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
		if( retry )
			Retry();
	}
	α ObjectsFolderAwait::Retry()ι->VoidAwait<>::Task{
		try{
			co_await AwaitSessionActivation( _ua );
			[]( ObjectsFolderAwait& self )->Task {
				try{
					auto j = co_await ObjectsFolderAwait{ self._node, self._snapshot, self._ua, self._sl };
					self.Resume( move(j) );
				}
				catch( IException& e ){
					self.ResumeExp( move(e) );
				}
			}( *this );
		}
		catch(IException& e){
			ResumeExp( move(e) );
		}
	}

	α OnResponse( UA_Client *ua, void* /*userdata*/, RequestId requestId, UA_BrowseResponse* response )ι->void{
		auto h = UAClient::ClearRequestH( ua, requestId ); if( !h ){ Critical( BrowseTag, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId ); return; }
		Trace( BrowseTag, "[{:x}.{}]OnResponse", (uint)ua, requestId );
		if( !response->responseHeader.serviceResult )
			Resume( ms<Response>(move(*response)), move(h) );
		else
			Resume( Client::UAClientException{response->responseHeader.serviceResult, ua, requestId}, move(h) );
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
	α Response::ToJson( up<flat_map<NodeId, Value>>&& pSnapshot, flat_map<NodeId, NodeId>&& dataTypes )ε->jobject{
		jarray references;
		for(size_t i = 0; i < resultsSize; ++i) {
			for(size_t j = 0; j < results[i].referencesSize; ++j) {
				UA_ReferenceDescription& ref = results[i].references[j];
				const NodeId nodeId{ move(ref.nodeId) };
				jobject reference;
				if( pSnapshot ){
					if( auto p = pSnapshot->find(nodeId); p!=pSnapshot->end() )
						reference["value"] = p->second.ToJson();
				}
				if( auto p = dataTypes.find(nodeId); p!=dataTypes.end() )
					reference["dataType"] = p->second.ToJson();
				else
					Warning( BrowseTag, "Could not find data type for node={}.", serialize(nodeId.ToJson()) );
				reference["referenceType"] = Opc::ToJson( ref.referenceTypeId );
				reference["isForward"] = ref.isForward;
				reference["node"] = nodeId.ToJson();

				jobject bn;
				const UA_QualifiedName& browseName = ref.browseName;
				bn["ns"] = browseName.namespaceIndex;
				bn["name"] = ToSV( browseName.name );
				reference["browseName"] = bn;

				jobject dn;
				const UA_LocalizedText& displayName = ref.displayName;
				dn["locale"] = ToSV( displayName.locale );
				dn["text"] = ToSV( displayName.text );
				reference["displayName"] = dn;

				reference["nodeClass"] = ref.nodeClass;
				reference["typeDefinition"] = Opc::ToJson( ref.typeDefinition );

				references.push_back( reference );
			}
		}
		jobject j;
		j["refs"] = references;
		return j;
	}
}