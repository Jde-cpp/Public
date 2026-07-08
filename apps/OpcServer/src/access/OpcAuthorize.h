#pragma once
#include <jde/access/Authorize.h>

namespace Jde::Opc::Server{
//	struct Listener; struct Loader; struct Permission;
	enum class EAccess : uint8{
		None			= 0,
		Read			= UA_ACCESSLEVELMASK_READ,
		Write			= UA_ACCESSLEVELMASK_WRITE,
		HistoryRead		= UA_ACCESSLEVELMASK_HISTORYREAD,
		HistoryWrite	= UA_ACCESSLEVELMASK_HISTORYWRITE,
		SemanticChange	= UA_ACCESSLEVELMASK_SEMANTICCHANGE,
		StatusWrite		= UA_ACCESSLEVELMASK_STATUSWRITE,
		TimestampWrite	= UA_ACCESSLEVELMASK_TIMESTAMPWRITE,
		All				= Read | Write | HistoryRead | HistoryWrite | SemanticChange | StatusWrite | TimestampWrite
	};

	struct OpcAuthorize final: Access::Authorize{
		OpcAuthorize( string app )ι:Access::Authorize{move(app)}{}
		α UserRights( NodeId nodeId, UserPK executer )ι->EAccess;
		α AssignRights( UA_Server& server )ι->void;
	private:
		α AssignRights( const NodeId& nodeId, UA_Server& server, Access::ResourcePK resourcePK, const std::map<NodeId, Access::ResourcePK>& baseResources, std::set<NodeId>& visited )ι->void;
		std::map<NodeId, Access::ResourcePK> _nodeResources; shared_mutex _nodeResourcesMutex;
		bool _enabled{};//true once base resources are configured; when false the server is unauthorized and every node is fully accessible.
		Access::ResourcePK _rootResourcePK{};//resource covering the ObjectsFolder root; unmapped nodes inherit it rather than being granted all access.
	};
}