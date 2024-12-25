#include "GroupHook.h"
//#include <jde/db/awaits/RowAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/generators/Statement.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/ql.h>
#include <jde/access/Authorize.h>
#include "../accessInternal.h"

#define let const auto

namespace Jde::Access{
	α RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK> members )ι->void;
	struct GroupGraphQLAwait final : TAwait<jvalue>{
		GroupGraphQLAwait( const QL::TableQL& query, UserPK userPK, SRCE )ι:
			TAwait<jvalue>{ sl },
			Query{ query },
			UserPK{ userPK }
		{}
		α Suspend()ι->void override{ Select(); }
		QL::TableQL Query;
		Jde::UserPK UserPK;
	private:
		α Select()ι->QL::QLAwait::Task;
	};

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
			optional<jarray> members;
			if( statement ){
				for( let& [name,value] : Query.Args ){
					string groupName = name=="id"
						? "groupId"
						: name=="target" ? "group_target" : name;
					membersQL.Args[groupName] = value;
				}
				statement->Where = QL::ToWhereClause( membersQL, groupTable, membersQL.FindColumn("deleted")!=nullptr );
				//statement->Where.Remove( "is_group" );
				//statement->Where.Replace( "identities.", "identity_groups." );
				auto membersResult = co_await QL::QLAwait( move(membersQL), move(*statement), UserPK, _sl );
				if( membersResult.is_array() )
					members = move( membersResult.get_array() );
				else if( membersResult.is_object() )
					members = jarray{ move(membersResult.get_object()) };
			}
			Query.JsonName = Query.IsPlural() ? "identities" : "identity"; //from identityGroups, want distinct + nothing in identityGroups table except for members.
			Query.AddFilter( "is_group", true );
			auto groups = co_await QL::QLAwait( move(Query), UserPK, _sl );
			if( !members )
				Resume( jvalue{move(groups)} );

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
				group["members"] = groupMembers;
			};
			if( groups.is_array() ){
				for( auto& group : groups.as_array() )
					addMembers( group.as_object() );
			}
			else if( groups.is_object() ) /*vs null.*/
				addMembers( groups.get_object() );
			//Trace( ELogTags::Access, "{}", serialize(groups) );
			Resume( move(groups) );
		}
		catch( boost::system::system_error& e ){
			ResumeExp( CodeException{e.code(), ELogTags::Access, ELogLevel::Debug} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α GroupHook::Select( const QL::TableQL& query, UserPK userPK, SL sl )ι->HookResult{
		return query.JsonName.starts_with( "identityGroup" ) && find_if(query.Tables, [](let& t){ return t.JsonName=="members"; })!=query.Tables.end()
			? mu<GroupGraphQLAwait>( query, userPK, sl )
			: nullptr;
	}

	//{ mutation addIdentityGroup( "id":14, "memberId":[15,13] ) }
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

	α GroupHook::AddBefore( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		if( m.JsonName.starts_with("identityGroup") ){
			auto [groupPK, memberPKs] = AddRemoveArgs( m );
			try{
				Authorizer().TestAddGroupMember( groupPK, move(memberPKs) );
			}catch( IException& e ){
				return mu<QL::ExceptionAwait>( e.Move() );
			}
		}
		return {};
	}

	α GroupHook::AddAfter( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		if( m.JsonName.starts_with("identityGroup") ){
			auto [groupPK, memberPKs] = AddRemoveArgs( m );
			Authorizer().AddToGroup( groupPK, memberPKs );
		}
		return {};
	}

	α GroupHook::RemoveAfter( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		if( m.JsonName.starts_with("identityGroup") ){
			auto [groupPK, memberPKs] = AddRemoveArgs( m );
			Authorizer().RemoveFromGroup( groupPK, memberPKs );
		}
		return {};
	}
	α GroupHook::UpdateAfter( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult{
		if( m.TableName()!="identityGroups" )
			return {};
		GroupPK pk{ m.Id<GroupPK::Type>() };
		if( m.Type==QL::EMutationQL::Delete )
			Authorizer().DeleteGroup( pk );
		else if( m.Type==QL::EMutationQL::Restore )
			Authorizer().RestoreGroup( pk );
		return {};
	}
	α GroupHook::PurgeAfter( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult{
		if( m.TableName()!="identityGroups" )
			Authorizer().PurgeGroup( GroupPK{m.Id<GroupPK::Type>()} );
		return {};
	}
}
