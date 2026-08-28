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

	//Node ACLs are stored in the generic Access::ERights vocabulary - the same columns as every other resource, and what the
	//node-access page writes (node-access.ts: "NOT EAccess/EWriteAccess").  EAccess is open62541's UA_ACCESSLEVELMASK layout and
	//the two share no bit positions (ERights::Read=0x2 is UA WRITE), so the only way across is this translation - never a cast.
	//Policy:  Read covers the history read, Update the value and history writes, Delete the history write, Administer the
	//status/timestamp/semantic-change bits;  Create, Purge, Subscribe and Execute have no node-level bit (methods go through
	//GetUserExecutable, a subscription needs Read).  ERights::All comes out as EAccess::All.  access-review3 #4.
	constexpr α ToAccess( Access::ERights rights )ι->EAccess{
		using enum Access::ERights;
		uint8 y{};
		auto add = [&]( Access::ERights right, EAccess access ){ if( !empty(rights & right) ) y |= underlying(access); };
		add( Read, EAccess::Read | EAccess::HistoryRead );
		add( Update, EAccess::Write | EAccess::HistoryWrite );
		add( Delete, EAccess::HistoryWrite );
		add( Administer, EAccess::StatusWrite | EAccess::TimestampWrite | EAccess::SemanticChange );
		return (EAccess)y;
	}

	struct OpcAuthorize final: Access::Authorize{
		OpcAuthorize( string app )ι:Access::Authorize{move(app)}{}
		α UserRights( NodeId nodeId, UserPK executer )ι->EAccess;
		α AssignRights( UA_Server& server )ι->void;
		//The AppServer's delegated admin check (ServerSocketSession::TestAdminAwait → OpcServerQL's adminCheck):  who may grant on a node is
		//whoever administers the resource governing it - the nearest configured ancestor's, else root - the same resolution
		//UserRights applies, which only this server can make.  Other targets take the generic flat rule.  Throws AccessException
		//on denial;  a plain Exception before AssignRights has run, so a check that races startup (the socket registers before
		//Configure and AssignRights) is a denial, never a guess.
		α TestAdminNode( str target, str criteria, UserPK user, SRCE )ε->void;
	private:
		α AssignRights( const NodeId& nodeId, UA_Server& server, Access::ResourcePK resourcePK, const std::map<NodeId, Access::ResourcePK>& baseResources, std::set<NodeId>& visited )ι->void;
		std::map<NodeId, Access::ResourcePK> _nodeResources; shared_mutex _nodeResourcesMutex;
		bool _enabled{};//true once base resources are configured; when false the server is unauthorized and every node is fully accessible.
		std::atomic<bool> _assigned{};//AssignRights has run (with or without base resources) - TestAdminNode denies until then.
		Access::ResourcePK _rootResourcePK{};//resource covering the ObjectsFolder root; unmapped nodes inherit it rather than being granted all access.
	};
}