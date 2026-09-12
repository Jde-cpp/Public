#pragma once
#include <jde/access/access.h>
#include <jde/ql/QLAwait.h>

namespace Jde::Access::Server{
	//userRights( id: ){ resource{id schemaName slug criteria deleted} allowed denied effective sources{ permissionId allowed denied path{id type} } }
	//One user's rights source by source, from the server's cache (Authorize::UserRights) - so the numbers are the ones Test and
	//Rights enforce.  A system table (Configure registers the name):  no view, so the shape is fixed and emitted whole as
	//adminCheck's is - TableQL::TrimColumns cannot keep array children, so the selection set is not applied.  Rights are the
	//numeric flags, as acl( identityId: ){ permissionRight{allowed} } returns them.  Group and role names are not cached - the
	//path carries pks and a `type` naming the list to resolve them against.
	//Gate:  the executer reads their own row freely, anyone else's takes Read on `acl`, the gate acl( identityId: ) has.
	struct UserRightsAwait final : TAwait<jvalue>, noncopyable{
		UserRightsAwait( const QL::TableQL& query, UserPK executer, SRCE )ι:
			TAwait<jvalue>{ sl },
			_query{ query },
			_executer{ executer }
		{}
		α Suspend()ι->void override;
	private:
		QL::TableQL _query;
		Jde::UserPK _executer;
	};
}
