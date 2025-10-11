#include "Browse.h"
#include <jde/fwk/process/execution.h>
#include <jde/ql/types/TableQL.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/LocalizedText.h>
#include <jde/opc/uatypes/Value.h>
#include "../UAClient.h"
#include "../async/Attributes.h"
#include "../async/ReadAwait.h"
#include "../async/SessionAwait.h"

#define let const auto
namespace Jde::Opc::Gateway{
	UABrowsePath::UABrowsePath( std::span<const sv> segments, NsIndex defaultNS )ι:
		UA_BrowsePath{
			UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
			{ segments.size(), (UA_RelativePathElement*)UA_Array_new(segments.size(), &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]) }
		}{
		for( size_t i=0; i<segments.size(); ++i ){
			auto elem = &relativePath.elements[i];
			elem->referenceTypeId = UA_NODEID_NUMERIC( 0, UA_NS0ID_ORGANIZES );
			auto ns = defaultNS;
			string path{ segments[i] };
			if( let nsPath = Str::Split( segments[i], '~' ); nsPath.size()>1 ){
				auto specifiedNs = Str::TryTo<NsIndex>( string{nsPath[0]} );
				if( specifiedNs ){
					ns = *specifiedNs;
					path = Str::Join( std::span{nsPath}.subspan(1), "~" );
				}
			}
			elem->includeSubtypes = elem->isInverse = false;
			elem->targetName = {ns, AllocUAString(path)};
		}
	}
namespace Browse{
	α FoldersAwait::Suspend()ι->void{
		_client->SendBrowseRequest( move(_request), _h );
	}
}
	ObjectsFolderAwait::ObjectsFolderAwait( NodeId node, bool snapshot, sp<UAClient> ua, SL sl )ι:
		base{ sl },
		_ua{ua},
		_node{node},
		_snapshot{snapshot}
	{}


	α ObjectsFolderAwait::Execute()ι->TAwait<Browse::Response>::Task{
		bool retry{};
		try{
			auto response = co_await Browse::FoldersAwait{ _node, UA_BROWSERESULTMASK_ALL, _ua };
			THROW_IF( response.Nodes().size()==0, "No items found for: {}", _node.ToString() );
			if( _snapshot )
				Snapshot( move(response) );
			else
				Attributes( response.Variables(), move(response) );
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


	α ObjectsFolderAwait::Snapshot( Browse::Response response )ι->TAwait<flat_map<NodeId, Value>>::Task{
		try{
			if( !_ua->Connected ){
				_ua = UAClient::Find( _ua->Target(), _ua->Credential );
				THROW_IF( !_ua, "Could not find UAClient for: {}", _ua->Target() );
			}
			auto vars = response.Variables();
			auto values = vars.size() ? co_await ReadAwait{ vars, _ua } : flat_map<NodeId, Value>{};
			Attributes( move(vars), move(response), move(values) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ObjectsFolderAwait::Attributes( flat_set<NodeId>&& variables, Browse::Response response, flat_map<NodeId, Value> values )ι->TAwait<flat_map<NodeId, NodeId>>::Task{
		try{
			auto dataTypes = variables.size() ? co_await AttribAwait{ move(variables), move(_ua) } : flat_map<NodeId, NodeId>{};
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
	α OnResponse( UA_Client *ua, void* userdata, RequestId requestId, UA_BrowseResponse* response )ι->void {
		auto h = UAClient::ClearRequestH<Browse::FoldersAwait::Handle>( ua, requestId );
		let uaHandle = (Handle)(userdata ? userdata : ua);
		if( !h ){
			CRITICALT( BrowseTag, "[{:x}.{:x}]Could not find handle.", uaHandle, requestId );
			return;
		}
		TRACET( BrowseTag, "[{:x}.{}]OnResponse", uaHandle, requestId );
		if( !response->responseHeader.serviceResult )
			Post<Response>( move(*response), move(h) ); // Cannot run EventLoop from the run method itself
		else
			h.promise().ResumeExp( UAClientException{response->responseHeader.serviceResult, uaHandle, requestId}, h );
	}

	flat_map<string, UA_BrowseResultMask> _attributes = {
		{"none", UA_BROWSERESULTMASK_NONE},
		{"browse", UA_BROWSERESULTMASK_BROWSENAME},
		{"isForward", UA_BROWSERESULTMASK_ISFORWARD},
		{"name", UA_BROWSERESULTMASK_DISPLAYNAME},
		{"nodeClass", UA_BROWSERESULTMASK_NODECLASS},
		{"refType", UA_BROWSERESULTMASK_REFERENCETYPEID},
		{"typeDef", UA_BROWSERESULTMASK_TYPEDEFINITION},
	};

	Request::Request( NodeId&& id, UA_BrowseResultMask mask )ι:
		UA_BrowseRequest{.requestedMaxReferencesPerNode=0, .nodesToBrowseSize=1, .nodesToBrowse=UA_BrowseDescription_new()}{
		nodesToBrowse[0].nodeId = move( id );
	 	nodesToBrowse[0].resultMask = mask;
	}

	α calcMask( const QL::TableQL& ql )ι->UA_BrowseResultMask{
		UA_BrowseResultMask mask = UA_BROWSERESULTMASK_NONE;
		for( let& c : ql.Columns ){
			if( auto attrib = _attributes.find(c.JsonName); attrib!=_attributes.end() )
				mask |= attrib->second;
		}
		return mask;
	}
	Request::Request( NodeId&& id, const QL::TableQL& ql )ι:
		Request( move(id), calcMask(ql) )
	{}

	α Response::operator=( Response&& x )ι->Response&{
		UA_BrowseResponse_clear( this );
		UA_BrowseResponse_copy( &x, this );
		UA_BrowseResponse_init( &x );
		return *this;
	}

	α Response::Nodes()Ι->flat_set<NodeId>{
		flat_set<NodeId> y;
		for( uint i = 0; i < resultsSize; ++i) {
      for( size_t j = 0; j < results[i].referencesSize; ++j )
				y.emplace( results[i].references[j].nodeId.nodeId );
		}
		return y;
	}

	α Response::Variables()Ι->flat_set<NodeId>{
		flat_set<NodeId> y;
		for( let& result : Iterable<UA_BrowseResult>(results, resultsSize) ){
			for( let& ref : Iterable<UA_ReferenceDescription>(result.references, result.referencesSize) ){
				if( ref.nodeClass == UA_NODECLASS_VARIABLE )
					y.emplace( move(ref.nodeId.nodeId) );
			}
		}
		return y;
	}
	α Response::VisitWhile( uint resultsIndex, function<bool(const UA_ReferenceDescription& ref)> f )Ι->bool{
		THROW_IF( resultsIndex>=resultsSize, "resultsIndex {} out of range {}.", resultsIndex, resultsSize );
		bool returnedFalse{};
		for( size_t j = 0; j < results[resultsIndex].referencesSize; ++j ){
			returnedFalse = !f(results[resultsIndex].references[j]);
			if( returnedFalse )
				break;
		}
		return !returnedFalse; //Returns false if f ever returns false.
	}
	α Response::SetJson( flat_map<NodeId, jobject>& children, bool addId )Ι->void{
		VisitWhile( 0, [&, addId=addId]( const UA_ReferenceDescription& ref ){
			jobject o;
			if( Attribs & UA_BROWSERESULTMASK_BROWSENAME )
				o["browse"] = BrowseName::ToJson( ref.browseName );
			if( Attribs & UA_BROWSERESULTMASK_ISFORWARD )
				o["isForward"] = ref.isForward;
			if( Attribs & UA_BROWSERESULTMASK_DISPLAYNAME )
				o["displayName"] = LocalizedText::ToJson( move(ref.displayName) );
			if( Attribs & UA_BROWSERESULTMASK_NODECLASS )
				o["nodeClass"] = ref.nodeClass;
			if( Attribs & UA_BROWSERESULTMASK_REFERENCETYPEID )
				o["refType"] = Opc::ToJson( ref.referenceTypeId );
			if( Attribs & UA_BROWSERESULTMASK_TYPEDEFINITION )
				o["typeDef"] = Opc::ToJson( ref.typeDefinition );
			NodeId nodeId{ move(ref.nodeId.nodeId) };
			if( addId )
				nodeId.Add( o );
			children.emplace( move(nodeId), o );
			return true;
		} );
	}
	α Response::ToJson( flat_map<NodeId, Value>&& snapshot, flat_map<NodeId, NodeId>&& dataTypes )ε->jobject{
		jarray references;
		for(size_t i = 0; i < resultsSize; ++i) {
			for(size_t j = 0; j < results[i].referencesSize; ++j) {
				UA_ReferenceDescription& ref = results[i].references[j];
				const NodeId nodeId{ move(ref.nodeId.nodeId) };
				jobject reference;
				if( auto p = snapshot.find(nodeId); p!=snapshot.end() )
					reference["value"] = p->second.ToJson();
				if( auto p = dataTypes.find(nodeId); p!=dataTypes.end() )
					reference["dataType"] = p->second.ToJson();
				reference["refType"] = Opc::ToJson( ref.referenceTypeId );
				reference["isForward"] = ref.isForward;
				reference["node"] = nodeId.ToJson();

				jobject bn;
				const UA_QualifiedName& browseName = ref.browseName;
				bn["ns"] = browseName.namespaceIndex;
				bn["name"] = ToSV( browseName.name );
				reference["browse"] = bn;

				jobject dn;
				const UA_LocalizedText& displayName = ref.displayName;
				dn["locale"] = ToSV( displayName.locale );
				dn["text"] = ToSV( displayName.text );
				reference["displayName"] = dn;

				reference["nodeClass"] = ref.nodeClass;
				reference["typeDef"] = Opc::ToJson( ref.typeDefinition );

				references.push_back( reference );
			}
		}
		jobject j;
		j["refs"] = references;
		return j;
	}
}}