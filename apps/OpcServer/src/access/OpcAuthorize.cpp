#include "OpcAuthorize.h"
#include <jde/fwk/utils/Stopwatch.h>

#define let const auto

namespace Jde::Opc::Server{
	constexpr ELogTags _tags{ ( ELogTags )( (EOpcLogTags)ELogTags::Access | EOpcLogTags::Opc ) };

	α OpcAuthorize::AssignRights( const NodeId& nodeId, UA_Server& server, Access::ResourcePK resourcePK, const std::map<NodeId, Access::ResourcePK>& baseResources )ι->void{
		if( auto it = baseResources.find(nodeId); it!=baseResources.end() )
			resourcePK = it->second;
		UA_BrowseDescription bd{
			nodeId,
			UA_BROWSEDIRECTION_FORWARD,
			UA_NODEID_NUMERIC( 0, UA_NS0ID_ORGANIZES ),
			UA_TRUE,
			UA_UINT32_MAX,
			UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD
		};
		auto br = UA_Server_browse( &server, UA_UINT32_MAX, &bd );
		if( br.statusCode ){
			UAException{ br.statusCode, Ƒ("Could not browse node {}", nodeId.ToString()) };
			return;
		}
		for( uint i=0; i<br.referencesSize; ++i ){
			auto ref = br.references[i];
			NodeId childNodeId{ ref.nodeId.nodeId };
			if( _nodeResources.find(childNodeId)!=_nodeResources.end() )
				continue;
			if( let it = baseResources.find({childNodeId}); it!=baseResources.end() ){
				resourcePK = it->second;
				TRACE( "[{}]resource:{}", childNodeId.ToString(), it->second );
			}
			if( resourcePK )
				_nodeResources.emplace( childNodeId, resourcePK );
			if( ref.nodeClass==UA_NODECLASS_OBJECT )
				AssignRights( childNodeId, server, resourcePK, baseResources );
		}
		UA_BrowseResult_clear(&br);
	}

	α OpcAuthorize::AssignRights( UA_Server& server )ι->void{
		Stopwatch sw{ "OpcAuthorize::AssignRights", _tags };
		sl _{ Mutex };
		std::map<NodeId, Access::ResourcePK> baseResources;
		const NodeId root=NodeId::ObjectsFolder();
		for( let& [pk,resource] : Resources ){
			if( resource.Target!="nodeIds" || resource.Schema!=_app )
				continue;
			try{
				baseResources.emplace( resource.Criteria.empty() ? root : NodeId::DecodeJson(resource.Criteria), pk );
			}
			catch( exception& e ){
				ERR( "Invalid NodeId '{}' for permission {}: {}", resource.Criteria, pk, e.what() );
				if( auto jde = dynamic_cast<IException*>(&e); jde )
					jde->SetLevel( ELogLevel::NoLog );
			}
		}
		if( baseResources.empty() ){
			DBG( "No base resources found for OPC UA server authorization." );
			return;
		}
		Access::ResourcePK rootResourcePK;
		ul _2{ _nodeResourcesMutex };
		if( let it = baseResources.find(root); it!=baseResources.end() ){
			rootResourcePK = it->second;
			_nodeResources.emplace( root, rootResourcePK );
			TRACE( "[{}]resource: {}", root.ToString(), rootResourcePK );
		}
		AssignRights( root, server, rootResourcePK, baseResources );
	}

	α OpcAuthorize::UserRights( NodeId nodeId, UserPK executer )ι->EAccess{
		optional<Access::ResourcePK> resourcePK;
		{
			sl _{ _nodeResourcesMutex };
			resourcePK = Find( _nodeResources, nodeId );
			if( !resourcePK )
				return EAccess::All; //not enabled.
		}

		sl _{ Mutex };
		auto user = Users.find( executer );
		if( user==Users.end() || user->second.IsDeleted )
			return EAccess::None;
		auto rights = user->second.ResourceRights( *resourcePK );
		return ( EAccess )rights.Effective();
	}
}