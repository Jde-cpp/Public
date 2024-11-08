#include "IdentityGroup.h"
#include <jde/db/await/RowAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/ql.h>

#define let const auto

namespace Jde::Access{
	α GetTable( str name )ι->sp<DB::Table>;

	Ω select( QL::TableQL query, GroupPK groupPK, UserPK userPK, HCoroutine h, SRCE )ι->QL::QLAwait::Task{
		try{
			//group_id, member_id & member columns.
			QL::TableQL membersQL = [&]()->QL::TableQL {
				auto p = find_if( query.Tables, [](let& t){ return t.JsonName=="members"; } );
				THROW_IF( p==query.Tables.end(), "members table not found." );
				auto ql = *p;
				query.Tables.erase( p );
				return ql;
			}();
			membersQL.Columns.push_back( QL::ColumnQL{"group_id"} );
			membersQL.Columns.push_back( QL::ColumnQL{"member_id"} );
			membersQL.JsonName = "groupMembers";
			auto statement = QL::SelectStatement( membersQL );
			jobject members;
			if( statement ){
				statement->Where = QL::ToWhereClause( membersQL, *GetTable("identities"), membersQL.FindColumn("deleted")!=nullptr );
				statement->Where.Replace( "identities.", "identityGroups." );
				members = co_await QL::QLAwait( move(membersQL), statement, sl );
			}

			groupOnlyQL.Columns.push_back( QL::ColumnQL{"member_id"} );
			jobject groups = co_await QL::QLAwait( move(groupOnlyQL), userPK, sl );
			if( !membersQL )
				Resume( mu<jobject>(move(groups)), h );

			jarray groupPKs;
			if( groupOnlyQL.IsPlural() ){
				for( let& v : Json::AsArrayPath( groups, "data/groups") )
					groupPKs.emplace_back( Json::AsNumber<GroupPK>(Json::AsObject(v), "id") );
			}
			else
				groupPKs.emplace_back( Json::AsNumber<GroupPK>(groups, "data/group/id") );
			let members = Json::AsArray( co_await QL::QLAwait(move(*membersQL), userPK, sl), "data/identities" );
			flat_map<IdentityPK,jobject> identities;
			for( let& member : members )
				identities.emplace( Json::AsNumber<GroupPK>(Json::AsObject(member), "id"), move(member) );

			for( let& member : members ){
				auto groupPK = Json::AsNumber<GroupPK>( Json::AsObject(member), "group/id" );
				groupMembers[groupPK].emplace_back( move(member) );
			}

			if( groupOnlyQL.IsPlural() ){
				for( let& group : Json::AsArrayPath(groups, "data/groups") ){
					auto groupPK = Json::AsNumber<GroupPK>( Json::AsObject(group), "id" );

					groupPKs.emplace_back( Json::AsNumber<GroupPK>(Json::AsObject(v), "id") );
			}
			else
				groupPKs.emplace_back( Json::AsNumber<GroupPK>(groups, "data/group/id") );

			auto Json::AsObject(groups, "data"); ["members"] = Json::AsArray( members, "data/identities" );
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