#pragma once
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/coroutine/TaskOld.h>

namespace Jde::DB{ struct AppSchema; }

namespace Jde::Access{
	struct Group final{
		Group( GroupPK id, bool isDeleted )ι:Id(id),IsDeleted(isDeleted){}
		GroupPK Id;
		bool IsDeleted;
		flat_set<IdentityPK> Members;
	};

	α IsChild( const flat_map<GroupPK,Group>& members, GroupPK parentPK, GroupPK childPK )ι->bool;
}