#include "IdentityGroup.h"
#include <jde/db/await/RowAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/types/TableQL.h>

#define let const auto

namespace Jde::Access{
	α GetTable( str name )ι->sp<DB::Table>;

	Ω select( const QL::TableQL& query, GroupPK groupPK, UserPK userPK, HCoroutine h, SRCE )ι->DB::RowAwait::Task{
		try{
			//find members and remove.
			auto groupOnlyQL = query;
			groupOnlyQL.Tables.erase( find_if(query.Tables, [](let& t){ return t.JsonName=="members"; }) );
			jobject y;
			[]( auto&& ql, auto userPK, auto& result, auto sl )->Coroutine::Task{
				try{
					result = co_await QL::QueryAwait( move(ql), userPK, sl );
				}
				catch( IException& e ){
					Resume( move(e), h );
				}
			}( move(groupOnlyQL), userPK, y, sl );
			//run regular ql
			//run query on members.
			//add members to output.
			let& extension = GetTable( "identities" );
			auto pk = extension->GetPK();
			let& users = GetTable( "users" );
			DB::Statement statement;
			statement.From+={ pk, groups->GetColumnPtr("member_id"), true };
			statement.From+={ groups->SurrogateKeys[0], pk, true, "groups_" };
			statement.Where.Add( pk, DB::Value{pk->Type, Json::AsNumber<GroupPK>(query.Args, "id")} );
			statement.Where.Add( extension->GetColumnPtr("deleted"), DB::Value{} );
			auto groupTable = find_if( query.Tables, [](let& t){ return t.JsonName=="identityGroups";} );
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
	struct GroupGraphQLAwait final: AsyncAwait{
		GroupGraphQLAwait( const QL::TableQL& query, GroupPK groupPK_, SRCE )ι:
			AsyncAwait{
				[&, groupPK=groupPK_]( HCoroutine h ){ select( query, groupPK, h, _sl ); },
				sl, "GroupGraphQLAwait" }
		{}
	};

	α GroupGraphQL::Select( const QL::TableQL& query, GroupPK groupPK, SL sl )ι->up<IAwait>{
		return query.JsonName.starts_with( "groupIdentit" ) && find_if(query.Tables, [](const auto& t){ return t.JsonName=="members"; })!=query.Tables.end()
			? mu<GroupGraphQLAwait>( query, groupPK, sl )
			: nullptr;
	}
}