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

	struct GroupGraphQLAwait final : TAwait<jvalue>{
		GroupGraphQLAwait( const QL::TableQL& query, UserPK userPK, SRCE )ι:
			TAwait<jvalue>{ sl },
			Query{ query },
			UserPK{ userPK }
		{}
		α Suspend()ι->void override{ Select(); }
		QL::TableQL Query;
		Access::UserPK UserPK;
	private:
		α Select()ι->QL::QLAwait::Task;
	};

	α GroupGraphQL::Select( const QL::TableQL& query, UserPK userPK, SL sl )ι->up<TAwait<jvalue>>{
		return query.JsonName.starts_with( "identityGroup" ) && find_if(query.Tables, [](const auto& t){ return t.JsonName=="members"; })!=query.Tables.end()
			? mu<GroupGraphQLAwait>( query, userPK, sl )
			: nullptr;
	}

	α GroupGraphQLAwait::Select()ι->QL::QLAwait::Task{
		try{
			//group_id, member_id & member columns.
			QL::TableQL membersQL = [&]()->QL::TableQL {
				auto p = find_if( Query.Tables, [](let& t){ return t.JsonName=="members"; } ); THROW_IF( p==Query.Tables.end(), "members table not found." );
				auto ql = *p;
				Query.Tables.erase( p );
				return ql;
			}();
			let& groupTable = *GetTable( "group_members" );
			let haveId = membersQL.FindColumn( "id" );
			if( haveId )
				membersQL.EraseColumn( "id" );
			membersQL.Columns.push_back( QL::ColumnQL{"groupId", groupTable.GetColumnPtr("group_id")} );
			membersQL.Columns.push_back( QL::ColumnQL{"memberId", groupTable.GetColumnPtr("member_id")} );
			membersQL.JsonName = "groupMembers";
			auto statement = QL::SelectStatement( membersQL );
			jobject membersResult;
			if( statement ){
				for( let& [name,value] : Query.Args )
					membersQL.Args[ name=="id" ? "groupId" : name] = value;
				statement->Where = QL::ToWhereClause( membersQL, groupTable, membersQL.FindColumn("deleted")!=nullptr );
				//statement->Where.Remove( "is_group" );
				//statement->Where.Replace( "identities.", "identity_groups." );
				membersResult = co_await QL::QLAwait( move(membersQL), move(*statement), UserPK, _sl );
			}
			auto& members = membersResult.at( "data" ).at( "groupMembers" ).as_array();
			jobject groups = co_await QL::QLAwait( move(Query), UserPK, _sl );
			if( !statement )
				Resume( jvalue{move(groups)} );

			auto addMembers = [&](jobject& group){
				let groupPK = Json::FindNumber<GroupPK>( group, "id" );//did not ask for group_id.
				jarray groupMembers;
				for( auto&& memberValue : members ){
					auto& member = memberValue.as_object();
					let memberGroupPK = Json::AsNumber<IdentityPK>( member, "groupId" );
					if( !groupPK || memberGroupPK==groupPK ){
						member.erase( "groupId" );
						if( haveId )
							member["id"] = Json::AsNumber<IdentityPK>( member, "memberId" );
						member.erase( "memberId" );
						groupMembers.emplace_back( move(member) );
					}
				}
				group["members"] = groupMembers;
			};
			auto& data = groups.at( "data" );
			jvalue value;
			if( auto jgroups = data.try_at_pointer("/identityGroups"); jgroups ){
				value = move(*jgroups);
				for( auto& vGroup : value.as_array() )
					addMembers( vGroup.as_object() );
			}
			else if( auto vGroup = data.try_at_pointer("/identityGroup"); vGroup && vGroup->is_object() ){ /*vs null.*/
				value = move(*vGroup);
				addMembers( value.get_object() );
			}
			Trace( ELogTags::Access, "{}", serialize(groups) );
			Resume( move(value) );
		}
		catch( boost::system::system_error& e ){
			ResumeExp( CodeException{e.code(), ELogTags::Access, ELogLevel::Debug} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}