#include "UserHook.h"
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#include <jde/ql/QLAwait.h>
#include "../../accessInternal.h"
#include "../serverInternal.h"
#define let const auto
#pragma GCC diagnostic ignored "-Wdangling-reference"

namespace Jde::Access::Server{
	struct UserGraphQLAwait final : TAwait<jvalue>{
		UserGraphQLAwait( const QL::TableQL& query, UserPK executer, SRCE )ι:
			TAwait<jvalue>{ sl },
			Query{ query },
			Executer{ executer }
		{}
		α Suspend()ι->void override{ QueryGroups(); }
		QL::TableQL Query;
		UserPK Executer;
	private:
		α QueryGroups()ι->QL::QLAwait<jarray>::Task;
		α QueryTables( jarray groups )ι->QL::QLAwait<jvalue>::Task;
		α GroupStatement()->DB::Statement;
		flat_map<uint8,string> _groupColumns;
	};
	α UserGraphQLAwait::GroupStatement()->DB::Statement{
		let& identityTable = GetTable( "identities" );
		auto pk = identityTable.GetPK();
		let& groupDBTable = GetTable( "groupings" );
		DB::Statement statement;
		auto& groupTable = Query.GetTable( "groupings" );
		let memberIdColumn = groupDBTable.GetColumnPtr( "member_id" );
		groupTable.Columns.emplace_back( QL::ColumnQL{"memberId", memberIdColumn} );

		statement.From+={ pk, memberIdColumn, true };
		statement.From+={ groupDBTable.SurrogateKeys[0], {}, pk, "groups_", true };
		if( let key = Query.FindArgKey(); key )
			statement.Where.Add( key->IsPrimary() ? pk : identityTable.GetColumnPtr("target"), DB::Value::FromKey(*key) );

		statement.Where.Add( identityTable.GetColumnPtr("deleted"), DB::Value{} );
		statement.Select.TryAdd( memberIdColumn );
		for( auto& c : groupTable.Columns ){
			if( c.JsonName=="memberId" ){
				c.DBColumn = memberIdColumn;
				statement.Select.TryAdd( c.DBColumn );
			}
			else{
				let dbColumn = c.JsonName=="id" ? pk : identityTable.GetColumnPtr( DB::Names::FromJson(c.JsonName) );
				/*c.DBColumn =*/ statement.Select.TryAdd( {"groups_", dbColumn} );
			}
		}
		return statement;
	}
	α UserGraphQLAwait::QueryGroups()ι->QL::QLAwait<jarray>::Task{
		try{
			auto groupStatement = GroupStatement();
			auto groupInfo = co_await QL::QLAwait<jarray>{ move(Query.GetTable("groupings")), move(groupStatement), Executer, _sl };
			QueryTables( move(groupInfo) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α UserGraphQLAwait::QueryTables( jarray groups )ι->QL::QLAwait<jvalue>::Task{
		try{
			Query.Tables.clear();
			//let returnRaw = Query.ReturnRaw;
			Query.ReturnRaw = true;
			auto userInfo = Query.Columns.size() ? co_await QL::QLAwait<jvalue>( move(Query), {}, Executer, _sl ) : jobject{};
			if( !userInfo.is_null() ){
				Json::Visit( userInfo, [&](jobject& user){
					let userPK = user.contains("id") ? QL::AsId<UserPK::Type>( user ) : optional<UserPK::Type>{};
					jarray userGroups;
					for( auto v=groups.begin(); v!=groups.end(); ){
						auto& group = v->as_object();
						if( let memberId = Json::AsNumber<UserPK::Type>( group, "memberId" ); !userPK || memberId==*userPK ){
							group.erase( "memberId" );
							userGroups.push_back( move(group) );
							v = groups.erase( v );
						}else
							v = std::next(v);
					}
					user["groupings"] = userGroups;
				} );
			}
			Resume( move(userInfo) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α UserHook::Select( const QL::TableQL& query, UserPK executer, SL sl )ι->up<TAwait<jvalue>>{
		return query.JsonName.starts_with( "user" ) && query.FindTable("groupings")
			? mu<UserGraphQLAwait>( query, executer, sl )
			: nullptr;
	}
}