#include "IdentityLoadAwait.h"
#include <jde/access/types/Group.h>
#include <jde/access/types/User.h>

#define let const auto

namespace Jde::Access{
	α IdentityLoadAwait::Load()ι->QL::QLAwait::Task{
		let values = co_await *_ql->Query( "identities{ id deleted is_group }", _executer, _sl );
		Identities identities;
		for( let value : Json::AsArray(values) ){
			let& identity = Json::AsObject(value);
			let isGroup = Json::AsBool(identity, "is_group");
			let pk{ Json::AsNumber<IdentityPK::Type>(identity, "id") };
			let deleted = Json::FindTimePoint( identity, "deleted" ).has_value();
			if( isGroup )
				identities.Groups.emplace( GroupPK{pk}, Group{GroupPK{pk}, deleted} );
			else
				identities.Users.emplace( UserPK{pk}, User{UserPK{pk}, deleted} );
		}
		let jgroups = co_await *_ql->Query( "identityGroups{ id deleted members{id} }", _executer, _sl );
		for( let value : Json::AsArray(jgroups) ){
			let& group = Json::AsObject(value);
			const GroupPK groupPK{ Json::AsNumber<GroupPK::Type>(group, "id") };
			auto p = identities.Groups.find(groupPK); THROW_IF( p==identities.Groups.end(), "[{}]Group not found", groupPK.Value );
			for( let member : Json::AsArray(group, "members") ){
				let memberPK{ Json::AsNumber<IdentityPK::Type>(Json::AsObject(member), "id") };
				p->second.Members.emplace( identities.Users.contains(UserPK{memberPK}) ? IdentityPK{UserPK{memberPK}} : IdentityPK{GroupPK{memberPK}} );
			}
		}
		Resume( std::move(identities) );
	}


/*	α IdentityLoadAwait::Execute()ι->DB::MapAwait<UserPK,optional<TimePoint>>::Task{
		let ql = "users{{ id deleted }}";

		let userTable{ _schema->GetTablePtr("identities") };
		let ds = userTable->Schema->DS();
		try{
			DB::SelectClause select{ {userTable->GetPK(), userTable->GetColumnPtr("deleted")} };
			DB::Statement statement{ move(select), {userTable}, {userTable->GetColumnPtr("is_group"), false} };
			auto users = co_await ds->SelectMap<UserPK,optional<TimePoint>>( statement.Move() );
			[]( auto& self, auto ds, auto users )ι->TAwait<flat_multimap<GroupPK,IdentityPK::Type>>::Task {
				try{
					Identities identities;
					for( let [pk,deleted] : users )
						identities.Users.emplace( pk, User{pk, deleted.has_value()} );
					let groupMembers = co_await *ds->template SelectMultiMap<GroupPK,IdentityPK::Type>( DB::SelectSKsSql(self._schema->GetTablePtr("identity_groups")) );
					for( auto&& [groupPk,memberPK] : groupMembers ){
						let user = identities.Users.find(UserPK{memberPK});
						IdentityPK pk = user==identities.Users.end() ? IdentityPK{GroupPK{memberPK}} : IdentityPK{UserPK{memberPK}};
						auto p = identities.GroupMembers.try_emplace( groupPk, flat_set<IdentityPK>{pk} );
						if( !p.second )
							p.first->second.emplace( pk );
					}
					self.Resume( std::move(identities) );
				}
				catch( IException& e ){
					self.ResumeExp( move(e) );
				}
			}( *this, ds, move(users) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
*/
}