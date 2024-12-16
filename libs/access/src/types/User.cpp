#include "User.h"
#include <jde/framework.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/awaits/RowAwait.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/TableQL.h>
#include "Permission.h"

#define let const auto
namespace Jde::Access{
	using namespace Jde::Coroutine;
	α User::UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		auto permission = Permissions.find(permissionPK);
		if( permission==Permissions.end() )
			return;
		let resourcePK = permission->second.ResourcePK;
		permission->second.Update( allowed, denied );
		AllowedDisallowed rights{ ERights::None, ERights::None };
		for( auto&& [_,p] : Permissions ){
			if( p.ResourcePK==resourcePK ){
				p.Update( allowed, denied );
				if( allowed )
					rights.Allowed |= *allowed;
				if( denied )
					rights.Denied |= *denied;
			}
		}
		Rights[resourcePK] = rights;
	}

	α User::operator+=( const Permission& permission )ι->User&{
		Permissions.insert_or_assign( permission.PK, permission );
		auto& rights = Rights[permission.ResourcePK];
		rights.Allowed |= permission.Allowed;
		rights.Denied |= permission.Denied;

		return *this;
	}

	struct UserGraphQLAwait final : TAwait<jvalue>{
		UserGraphQLAwait( const QL::TableQL& query, UserPK userPK, SRCE )ι:
			TAwait<jvalue>{ sl },
			Query{ query },
			UserPK{ userPK }
		{}
		α Suspend()ι->void override{ Select(); }
		QL::TableQL Query;
		Jde::UserPK UserPK;
	private:
		α Select()ι->DB::RowAwait::Task;
	};

	α UserGraphQLAwait::Select()ι->DB::RowAwait::Task{
		try{
			let& extension = GetTable( "identities" );
			auto pk = extension->GetPK();
			let& groups = GetTable( "identity_groups" );
			DB::Statement statement;
			statement.From+={ pk, groups->GetColumnPtr("member_id"), true };
			statement.From+={ groups->SurrogateKeys[0], pk, true, "groups_" };
			statement.Where.Add( pk, DB::Value{pk->Type, Json::AsNumber<Jde::UserPK::Type>(Query.Args, "id")} );
			statement.Where.Add( extension->GetColumnPtr("deleted"), DB::Value{} );
			auto groupTable = find_if( Query.Tables, [](let& t){ return t.JsonName=="identityGroups";} );
			flat_map<uint8,string> qlColumns;
			uint i=0;
			for( auto& c : groupTable->Columns ){
				if( let dbCol = groups->FindColumn( c.JsonName=="id" ? "identity_id" : DB::Names::FromJson(c.JsonName)); dbCol ){
					auto alias = ms<DB::Column>( *dbCol );
					alias->Table = ms<DB::Table>( "groups_" );
					statement.Select.TryAdd( alias );
					qlColumns.emplace( i++, c.JsonName );
				}
			}
			let rows = co_await extension->Schema->DS()->SelectCo( statement.Move(), _sl );
			jarray identityGroups;
			for( auto& row : rows ){
				jobject group;
				for( auto& [i, name] : qlColumns )
					group[name] = (*row)[i].ToJson();
				identityGroups.push_back( group );
			}
			jobject y;
			y["identityGroups"] = identityGroups;
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α UserGraphQL::Select( const QL::TableQL& query, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		return query.JsonName.starts_with( "user" ) && find_if(query.Tables, [](const auto& t){ return t.JsonName=="identityGroups"; })!=query.Tables.end()
			? mu<UserGraphQLAwait>( query, userPK, sl )
			: nullptr;
	}
}