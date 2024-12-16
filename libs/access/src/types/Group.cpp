#include "Group.h"

namespace Jde{

	α Access::IsChild( const flat_map<GroupPK,Group>& groups, GroupPK parentPK, GroupPK childPK )ι->bool{
		auto group = groups.find(parentPK); ASSERT( group!=groups.end() );
		if( group==groups.end() )
			return false;
		bool isChild{};
		for( auto member = group->second.Members.begin(); !isChild && member!=group->second.Members.end(); ++member )
			isChild = !member->IsUser() && ( member->GroupPK()==childPK || IsChild(groups, member->GroupPK(), childPK) );
		return isChild;
	}
}