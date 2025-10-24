#include "GroupHook.h"
//#include <jde/db/awaits/RowAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/QLAwait.h>
#include <jde/access/Authorize.h>
#include "../serverInternal.h"
#include "../../accessInternal.h"
#pragma GCC diagnostic ignored "-Wdangling-reference"

#define let const auto

namespace Jde::Access::Server{
	α RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK> members )ι->void;
	struct GroupGraphQLAwait final : TAwait<jvalue>{
		GroupGraphQLAwait( const QL::TableQL& query, UserPK executer, SRCE )ι:
			TAwait<jvalue>{ sl },
			Query{ query },
			Executer{ executer }
		{}
		α Suspend()ι->void override{ Select(); }
		QL::TableQL Query;
		Jde::UserPK Executer;
	private:
		α Select()ι->QL::QLAwait<>::Task;
	};

	α GroupGraphQLAwait::Select()ι->QL::QLAwait<>::Task{
		try{
			//group_id, member_id & member columns.
			optional<QL::TableQL> membersQL = [&]()->optional<QL::TableQL>{
				auto p = find_if( Query.Tables, [](let& t){ return t.JsonName.starts_with("groupMember"); } );
				if( p==Query.Tables.end() )
					return {};
				auto ql = *p;
				Query.Tables.erase( p );
				return ql;
			}();
			GetTable( "groupings" ).Authorize( Access::ERights::Read, Executer, _sl );
			optional<jarray> members;
			bool haveId{};
			Query.JsonName = Query.IsPlural() ? "identities" : "identity"; //from members, want distinct + nothing in members table except for members.
			Query.AddFilter( "is_group", true );
			Query.ReturnRaw = true;
			let onlyHaveId = Query.Columns.size()==1 && Query.Columns[0].JsonName=="id";
			auto groups = !onlyHaveId && Query.Columns.size() ? co_await QL::QLAwait( move(Query), {}, Executer, _sl ) : jobject{};
			if( membersQL ){
				let& groupTable = GetTable( "group_members" );
				haveId = membersQL->FindColumn( "id" );
				if( haveId )
					membersQL->EraseColumn( "id" );
				membersQL->Columns.push_back( QL::ColumnQL{"groupId", groupTable.GetColumnPtr("group_id")} );
				membersQL->Columns.push_back( QL::ColumnQL{"memberId", groupTable.GetColumnPtr("member_id")} );
				if( let idArg = membersQL->Args.if_contains("id"); idArg ){
					auto id = *idArg;
					membersQL->Args["memberId"] = id;
					membersQL->Args.erase( "id" );
				}
				membersQL->JsonName = "groupMembers";
				auto statement = QL::SelectStatement( *membersQL );
				if( statement ){
					for( let& [name,value] : Query.Args ){
						if( name=="is_group" )
							continue;
						string groupName = name=="id"
							? "groupId"
							: name=="target" ? "group_target" : name;
						membersQL->Args[groupName] = value;
					}
					statement->Where = QL::ToWhereClause( *membersQL, groupTable, membersQL->FindColumn("deleted")!=nullptr );
					//statement->Where.Remove( "is_group" );
					//statement->Where.Replace( "identities.", "members." );
					auto membersResult = co_await QL::QLAwait( move(*membersQL), {}, move(*statement), Executer, _sl );
					if( membersResult.is_array() )
						members = move( membersResult.get_array() );
					else if( membersResult.is_object() )
						members = jarray{ move(membersResult.get_object()) };
				}
			}
			if( !members ){
				Resume( jvalue{move(groups)} );
				co_return;
			}
			auto addMembers = [&](jobject& group){
				optional<GroupPK> groupPK = Json::FindKey<GroupPK>(group);//did not ask for group_id.
				jarray groupMembers;
				for( auto&& memberValue : *members ){
					auto& member = memberValue.as_object();
					const GroupPK memberGroupPK{ Json::FindNumber<GroupPK::Type>(member, "groupId").value_or(0) };//group already assigned
					if( !groupPK || memberGroupPK==*groupPK ){
						member.erase( "groupId" );
						if( haveId )
							member["id"] = Json::AsNumber<IdentityPK::Type>( member, "memberId" );
						member.erase( "memberId" );
						groupMembers.emplace_back( move(member) );
					}
				}
				group["groupMembers"] = groupMembers;
			};
			if( !groups.is_null() )
				Json::Visit( groups, addMembers );
			Resume( move(groups) );
		}
		catch( boost::system::system_error& e ){
			ResumeExp( CodeException{e.code(), ELogTags::Access, ELogLevel::Debug} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α GroupHook::Select( const QL::TableQL& query, UserPK executer, SL sl )ι->HookResult{
		return query.JsonName.starts_with("grouping") ? mu<GroupGraphQLAwait>( query, executer, sl ) : nullptr;
	}

	//{ mutation addGrouping( "id":14, "memberId":[15,13] ) }
	α GroupHook::AddRemoveArgs( const QL::MutationQL& m )ι->std::pair<GroupPK, flat_set<IdentityPK::Type>>{
		const GroupPK groupPK{ Json::AsNumber<GroupPK::Type>(m.Args, "id") };
		flat_set<IdentityPK::Type> memberPKs;
		auto members = Json::FindValue( m.Args, "memberId" );
		if( members ){
			if( members->is_array() ){
				for( auto& member : members->get_array() ){
					memberPKs.emplace( Json::FindNumber<IdentityPK::Type>(member,{}).value_or(0) );
				}
			}else if( members->is_number() )
				memberPKs.emplace( Json::FindNumber<IdentityPK::Type>(*members,{}).value_or(0) );
		}
		return {groupPK, memberPKs};
	}

	α GroupHook::AddBefore( const QL::MutationQL& m, jobject /*variables*/, UserPK /*executer*/, SL sl )ι->HookResult{
		if( m.TableName()=="groupings" ){
			auto [groupPK, memberPKs] = AddRemoveArgs( m );
			try{
				Authorizer().TestAddGroupMember( groupPK, move(memberPKs), sl );
			}catch( IException& e ){
				return mu<QL::ExceptionAwait>( e.Move() );
			}
		}
		return {};
	}
}