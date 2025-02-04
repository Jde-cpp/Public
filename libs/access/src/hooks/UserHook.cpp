#include "UserHook.h"
#include <jde/db/IDataSource.h>
#include <jde/db/awaits/RowAwait.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#include "../accessInternal.h"
#define let const auto

namespace Jde::Access{
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
			let& groups = GetTable( "groupings" );
			DB::Statement statement;
			statement.From+={ pk, groups->GetColumnPtr("member_id"), true };
			statement.From+={ groups->SurrogateKeys[0], pk, true, "groups_" };
			statement.Where.Add( pk, DB::Value{pk->Type, Json::AsNumber<Jde::UserPK::Type>(Query.Args, "id")} );
			statement.Where.Add( extension->GetColumnPtr("deleted"), DB::Value{} );
			auto groupTable = find_if( Query.Tables, [](let& t){ return t.JsonName=="groupings";} );
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
			jarray jgroups;
			for( auto& row : rows ){
				jobject group;
				for( auto& [i, name] : qlColumns )
					group[name] = (*row)[i].ToJson();
				jgroups.push_back( group );
			}
			Resume( jobject{{"groupings", jgroups}} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α UserHook::Select( const QL::TableQL& query, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		return query.JsonName.starts_with( "user" ) && query.FindTable("groupings")
			? mu<UserGraphQLAwait>( query, userPK, sl )
			: nullptr;
	}
}