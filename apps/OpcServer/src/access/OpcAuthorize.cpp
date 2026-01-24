#include "OpcAuthorize.h"
//#include <jde/access/usings.h>

namespace Jde::Opc::Server{
	α OpcAuthorize::UserRights( NodeId nodeId, UserPK executer )ι->EAccess{
		string criteria = nodeId.ToString();
		Jde::sl _{Mutex};
		optional<Access::ResourcePK> resourcePK = FindResourcePK( _app, "nodeIds", criteria, _ );
		if( !resourcePK )//not enabled
			return EAccess::All;

		auto user = Users.find(executer);
		if( user==Users.end() || user->second.IsDeleted )
			return EAccess::None;
		auto rights = user->second.ResourceRights( *resourcePK );
		return (EAccess)rights.Allowed & ~((EAccess)rights.Denied);
	}
}