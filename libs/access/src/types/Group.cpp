#include <jde/access/types/Group.h>

namespace Jde{

	α Access::IsChild( const flat_map<GroupPK,Group>& members, GroupPK parentPK, GroupPK childPK )ι->bool{
		auto group = members.find(parentPK);
		if( group==members.end() )//memberless group.
			return false;
		bool isChild{};
		for( auto member = group->second.Members.begin(); !isChild && member!=group->second.Members.end(); ++member )
			isChild = !member->IsUser() && ( member->GroupPK()==childPK || IsChild(members, member->GroupPK(), childPK) );
		return isChild;
	}
}