#include "User.h"
#include <jde/framework.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/await/RowAwait.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/TableQL.h>

#define let const auto
namespace Jde::Access{
	using namespace Jde::Coroutine;

	UserLoadAwait::UserLoadAwait( sp<DB::AppSchema> schema )ι:
		_schema{ schema }
	{}

	α UserLoadAwait::Execute()ι->Coroutine::Task{
		let userTable{ _schema->GetTablePtr("users") };
		let ds = userTable->Schema->DS();
		try{
			auto pks = ( co_await ds->SelectSet<UserPK>(DB::SelectSKsSql(userTable)) ).UP<flat_set<UserPK>>();
			[this]( auto ds, auto pks )ι->TAwait<flat_multimap<GroupPK,UserPK>>::Task {
				try{
					let userGroups = co_await *ds->template SelectMultiMap<GroupPK,UserPK>( DB::SelectSKsSql(_schema->GetTablePtr("groups")) );
					concurrent_flat_map<UserPK,User> users;
					for( auto&& [groupPk,userPk] : userGroups )
						users.visit( userPk, [&](auto& kv){ kv.second.Groups.emplace(groupPk); } );
					Resume( std::move(users) );
				}
				catch( IException& e ){
					ResumeExp( move(e) );
				}
			}(ds, std::move(pks));
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α GetTable( str name )ι->sp<DB::Table>;

	Ω select( const QL::TableQL& query, UserPK userPK, HCoroutine h, SRCE )ι->DB::RowAwait::Task{
		try{
			let& extension = GetTable( "identities" );
			auto pk = extension->GetPK();
			let& groups = GetTable( "identity_groups" );
			DB::Statement statement;
			statement.From+={ pk, groups->GetColumnPtr("member_id"), true };
			statement.From+={ groups->SurrogateKeys[0], pk, true, "groups_" };
			statement.Where.Add( pk, DB::Value{pk->Type, Json::AsNumber<UserPK>(query.Args, "id")} );
			statement.Where.Add( extension->GetColumnPtr("deleted"), DB::Value{} );
			auto groupTable = find_if( query.Tables, [](const auto& t){ return t.JsonName=="identityGroups";} );
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
			let rows = co_await extension->Schema->DS()->SelectCo( statement.Move(), sl );
			jarray identityGroups;
			for( auto& row : rows ){
				jobject group;
				for( auto& [i, name] : qlColumns )
					group[name] = (*row)[i].ToJson();
				identityGroups.push_back( group );
			}
			jobject y;
			y["identityGroups"] = identityGroups;
			Resume( mu<jobject>( move(y) ), h );
		}
		catch( IException& e ){
			Resume( move(e), h );
		}

	}
	struct UserGraphQLAwait final: AsyncAwait{
		UserGraphQLAwait( const QL::TableQL& query, UserPK userPK_, SRCE )ι:
			AsyncAwait{
				[&, userPK=userPK_]( HCoroutine h ){ select( query, userPK, h, _sl ); },
				sl, "UserGraphQLAwait" }
		{}
	};

	α UserGraphQL::Select( const QL::TableQL& query, UserPK userPK, SL sl )ι->up<IAwait>{
		return query.JsonName.starts_with( "user" ) && find_if(query.Tables, [](const auto& t){ return t.JsonName=="identityGroups"; })!=query.Tables.end()
			? mu<UserGraphQLAwait>( query, userPK, sl )
			: nullptr;
	}
}