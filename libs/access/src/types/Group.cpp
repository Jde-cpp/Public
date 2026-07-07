#include <jde/access/types/Group.h>

namespace Jde{

	Ω isChild( const flat_map<Access::GroupPK,Access::Group>& members, Access::GroupPK parentPK, Access::GroupPK childPK, flat_set<Access::GroupPK>& visited )ι->bool{
		auto group = visited.emplace(parentPK).second ? members.find( parentPK ) : members.end();//visited guards cycles in existing data.
		if( group==members.end() )//memberless group.
			return false;
		bool y{};
		for( auto member = group->second.Members.begin(); !y && member!=group->second.Members.end(); ++member )
			y = !member->IsUser() && ( member->GroupPK()==childPK || isChild(members, member->GroupPK(), childPK, visited) );
		return y;
	}
	α Access::IsChild( const flat_map<GroupPK,Group>& members, GroupPK parentPK, GroupPK childPK )ι->bool{
		flat_set<Access::GroupPK> visited;
		return isChild( members, parentPK, childPK, visited );
	}
}