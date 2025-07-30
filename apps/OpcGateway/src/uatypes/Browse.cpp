#include "Browse.h"
#include "../UAClient.h"
#include <jde/opc/uatypes/Value.h>
#include "../async/Attributes.h"
#include "../async/ReadAwait.h"
#include "../async/SessionAwait.h"

namespace Jde::Opc::Gateway{
namespace Browse{
	α FoldersAwait::Suspend()ι->void{
		_client->SendBrowseRequest( Request{move(_node)}, _h );
	}
}
	ObjectsFolderAwait::ObjectsFolderAwait( ExNodeId node, bool snapshot, sp<UAClient> ua, SL sl )ι:
		base{ sl },
		_ua{ua},
		_node{node},
		_snapshot{snapshot}
	{}


	α ObjectsFolderAwait::Execute()ι->TAwait<Browse::Response>::Task{
		bool retry{};
		try{
			auto response = co_await Browse::FoldersAwait{ _node, _ua };
			THROW_IF( response.Nodes().size()==0, "No items found for: {}", _node.to_string() );
			if( _snapshot )
				Snapshot( move(response) );
			else
				Attributes( move(response) );
		}
		catch( UAClientException& e ){
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


	α ObjectsFolderAwait::Snapshot( Browse::Response response )ι->TAwait<flat_map<ExNodeId, Value>>::Task{
		try{
			auto values = co_await ReadAwait{ response.Nodes(), _ua };
			Attributes( move(response), move(values) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectsFolderAwait::Attributes( Browse::Response response, flat_map<ExNodeId, Value> values )ι->TAwait<flat_map<ExNodeId, ExNodeId>>::Task{
		try{
			auto dataTypes = co_await AttribAwait{ move(response.Nodes()), move(_ua) };
			Resume( response.ToJson(move(values), move(dataTypes)) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectsFolderAwait::Retry()ι->VoidAwait::Task{
		try{
			co_await AwaitSessionActivation( _ua );
			[]( ObjectsFolderAwait&& self )->Task{
				try{
					auto j = co_await ObjectsFolderAwait{ self._node, self._snapshot, self._ua, self._sl };
					self.Resume( move(j) );
				}
				catch( exception& e ){
					self.ResumeExp( move(e) );
				}
			}( move(*this) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
namespace Browse{
	α OnResponse( UA_Client *ua, void* /*userdata*/, RequestId requestId, UA_BrowseResponse* response )ι->void{
		auto h = UAClient::ClearRequestH<Browse::FoldersAwait::Handle>( ua, requestId ); if( !h ){ Critical( BrowseTag, "[{:x}.{:x}]Could not find handle.", (uint)ua, requestId ); return; }
		Trace( BrowseTag, "[{:x}.{}]OnResponse", (uint)ua, requestId );
		if( !response->responseHeader.serviceResult )
			h.promise().Resume( move(*response), h );
		else
			h.promise().ResumeExp( UAClientException{response->responseHeader.serviceResult, ua, requestId}, move(h) );
	}

	Request::Request( ExNodeId&& node )ι{
		UA_BrowseRequest_init( this );
    requestedMaxReferencesPerNode = 0;
    nodesToBrowse = UA_BrowseDescription_new();
    nodesToBrowseSize = 1;
    nodesToBrowse[0].nodeId = node.Move();
    nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
	}

	α Response::operator=( Response&& x )ι->Response&{
		UA_BrowseResponse_delete( this );
		UA_BrowseResponse_copy( &x, this );
		UA_BrowseResponse_clear( &x );
		return *this;
	}

	α Response::Nodes()ι->flat_set<ExNodeId>{
		flat_set<ExNodeId> y;
		for( uint i = 0; i < resultsSize; ++i) {
      for( size_t j = 0; j < results[i].referencesSize; ++j )
				y.emplace( results[i].references[j].nodeId );
		}
		return y;
	}
	α Response::ToJson( flat_map<ExNodeId, Value>&& snapshot, flat_map<ExNodeId, ExNodeId>&& dataTypes )ε->jobject{
		jarray references;
		for(size_t i = 0; i < resultsSize; ++i) {
			for(size_t j = 0; j < results[i].referencesSize; ++j) {
				UA_ReferenceDescription& ref = results[i].references[j];
				const ExNodeId nodeId{ move(ref.nodeId) };
				jobject reference;
				if( auto p = snapshot.find(nodeId); p!=snapshot.end() )
					reference["value"] = p->second.ToJson();
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
}}