#include "IdentityLoadAwait.h"
#include <jde/access/types/Group.h>
#include <jde/access/types/User.h>
#include <jde/ql/IQL.h>

#define let const auto

namespace Jde::Access{
	α IdentityLoadAwait::Load()ι->QL::QLAwait<jarray>::Task{
		try{
			let values = co_await *_ql->QueryArray( "identities{ id deleted is_group }", {}, _executer, true, _sl );
			Identities identities;
			for( let& value : values ){
				let& identity = Json::AsObject(value);
				let isGroup = Json::AsBool(identity, "is_group");
				let pk{ Json::AsNumber<IdentityPK::Type>(identity, "id") };
				let deleted = Json::FindTimePoint( identity, "deleted" ).has_value();
				if( isGroup )
					identities.Groups.emplace( GroupPK{pk}, Group{GroupPK{pk}, deleted} );
				else
					identities.Users.emplace( UserPK{pk}, User{UserPK{pk}, deleted} );
			}
			let jgroups = co_await *_ql->QueryArray( "groupings{ id deleted groupMembers{id} }", {}, _executer, true, _sl );
			for( let& value : jgroups ){
				let& group = Json::AsObject(value);
				const GroupPK groupPK{ Json::AsNumber<GroupPK::Type>(group, "id") };
				auto p = identities.Groups.find(groupPK); THROW_IF( p==identities.Groups.end(), "[{}]Group not found", groupPK.Value );
				for( let& member : Json::AsArray(group, "groupMembers") ){
					let memberPK{ Json::AsNumber<IdentityPK::Type>(Json::AsObject(member), "id") };
					p->second.Members.emplace( identities.Users.contains(UserPK{memberPK}) ? IdentityPK{UserPK{memberPK}} : IdentityPK{GroupPK{memberPK}} );
				}
			}
			Resume( std::move(identities) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}