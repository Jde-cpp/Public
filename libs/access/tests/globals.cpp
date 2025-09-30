#include "globals.h"
#include <jde/fwk/str.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>
#include "../src/awaits/ResourceLoadAwait.h"
#include <jde/access/Authorize.h>

#define let const auto

namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::Access{
	using namespace DB::Names;
	constexpr ELogTags _tags{ ELogTags::Test | ELogTags::Pedantic };
	sp<QL::LocalQL> _localQL;
	sp<Authorize> _authorizer = ms<Authorize>( "Tests" );

	α Tests::Authorizer()ι->sp<Access::Authorize>{ return _authorizer; }
	α Tests::QL()ι->QL::LocalQL&{ return *_localQL; }
	α Tests::QLPtr()ι->sp<QL::LocalQL>{ return _localQL; }
	α Tests::SetQL( sp<QL::LocalQL> ql )ι->void{ _localQL = move(ql); }
	α Tests::DS()ι->DB::IDataSource&{ return _localQL->DS(); }
	α Tests::GetTable( str name )ι->sp<DB::Table>{ return DB::AsTable(_localQL->GetTablePtr(FromJson(name)) ); }
	α Tests::Schemas()ι->vector<sp<DB::AppSchema>>{ return _localQL->Schemas(); }


namespace Tests{
	α CreateAcl( IdentityPK identityPK, ERights allowed, ERights denied, string resource, UserPK executer )ε->PermissionRightsPK;
	Ω testUnauthGet( str table, str target, UserPK executer, sv cols, bool includeDeleted )ε->jobject{
		auto y = Select( table, target, GetRoot(), cols, includeDeleted );
		if( y.empty() ){
			try{
				Create(table, target, executer);
				throw std::runtime_error( "Should not be able to create." );
			}
			catch( IException& e ){ e.SetLevel(ELogLevel::NoLog); }
			Create( table, target, GetRoot() );
			EXPECT_THROW( Select(table, target, executer, cols, includeDeleted), IException );
			y = Select( table, target, GetRoot(), cols, includeDeleted );
		}
		return y;
	}

	Ω updateNameQL( str table, uint id, sv updatedName )ε->string{
		return Str::Replace( Ƒ("mutation update{}( id:{}, name:'{}' )", Capitalize(table), id, updatedName), '\'', '"' );
	}
	Ω testUpdateName( str table, uint id, UserPK executer, sv updatedName )ε->void{
 		let updateJson = QL().QuerySync<jvalue>( updateNameQL(table, id, updatedName), executer );
		ASSERT_TRUE( Json::AsSV(Select(table,id, GetRoot(), {}, true), "name")==updatedName );
	}

	Ω deleteQL( str table, uint id )ι->string{ return Ƒ( "mutation delete{}( id:{} )", Capitalize(table), id ); }
	Ω restoreQL( str table, uint id )ι->string{ return Ƒ( "mutation restore{}( id:{} )", Capitalize(table), id ); }
	Ω testDeleteRestore( str table, uint id, UserPK executer )ε->void{
		let del = Ƒ( "mutation delete{}(\"id\":{})", Capitalize(table), id );
		let deleteJson = QL().QuerySync<jvalue>( deleteQL(table, id), executer );
		ASSERT_TRUE( Select(table, id, executer).empty() );
		ASSERT_FALSE( Select(table, id, executer, {}, true).empty() );

 		let restoreJson = QL().QuerySync<jvalue>( restoreQL(table, id), executer );
		ASSERT_FALSE( Select(table, id, executer).empty() );
	}
	Ω addRemoveQL( sv op, const DB::Table& table, uint pk, vector<uint> members )ε->string{
		let& map = *table.Map;
		let parentTable = map.Parent->Table;
		let parentTableName = Capitalize( parentTable->JsonName() );
		let memberString = members.size()==1 ? Ƒ( "{}", members[0] ) : '['+Str::Join( members )+']';
		return Ƒ( "mutation {}{}( id:{}, {}:{} )", op, parentTableName, pk, ToJson(map.Child->Name), memberString );
	}
	Ω addRemove( sv op, const DB::Table& table, uint pk, vector<uint> members, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( addRemoveQL(op, table, pk, members), executer );
	}
}
	α Tests::TestUnauthUpdateName( str table, uint id, UserPK executer, sv updatedName )ε->void{
 		EXPECT_THROW( QL().QuerySync<jvalue>( updateNameQL(table, id, updatedName), executer), IException );
	}
	α Tests::TestUnauthDeleteRestore( str table, uint id, UserPK executer )ε->void{
		EXPECT_THROW( QL().QuerySync<jvalue>( deleteQL(table,id), executer), IException );
		EXPECT_THROW( QL().QuerySync<jvalue>( restoreQL(table,id), executer), IException );
	}

	using namespace Json;

	α Tests::Add( const DB::Table& table, uint groupPK, vector<uint> members, UserPK executer )ε->void{
		addRemove( "add", table, groupPK, members, executer );
	}

	α memberString( vector<IdentityPK> members )ε->string{
		string memberString;
		for( let member : members )
			memberString+= std::to_string(member.Underlying()) + ',';
		memberString.pop_back();
		if( members.size()>1 )
			memberString = "["+memberString+"]";
		return memberString;
	}
	α Tests::AddToGroup( GroupPK id, vector<IdentityPK> members, UserPK executer )ε->void{
		let ql =	Ƒ( "mutation addGrouping( \"id\":{}, \"memberId\":{} )", id.Value, memberString(members) );
		let addJson = QL().QuerySync<jvalue>( ql, executer );
	}

	α Tests::Remove( const DB::Table& table, uint groupPK, vector<uint> members, UserPK executer )ε->void{
		addRemove( "remove", table, groupPK, members, executer );
	}
	α Tests::RemoveFromGroup( GroupPK id, vector<IdentityPK> members, UserPK executer )ε->void{
		let ql = Ƒ( "mutation removeGrouping( \"id\":{}, \"memberId\":{} )", id.Value, memberString(members) );
		let removeJson = QL().QuerySync<jvalue>( ql, executer );
	}


	α Tests::Create( str table, sv target, UserPK executer, str input, SL sl )ε->uint{
		let create = Ƒ( "create{0}(  target:'{1}', name:'{1} - name', description:'{1} - description' {2} ){{id}}", Capitalize(table), target, input.size() ? ","s+input : "" );
		let createJson = QL().QuerySync( Str::Replace(create, '\'', '"'), executer, true, sl );
		return GetId( createJson );//{"user":{"id":7}}
	}
	using Tests::QL;
	Ω createUser( str target, ProviderPK providerId, UserPK executer )ε->UserPK{
		let create = Ƒ( "createUser(  loginName:'{0}', target:'{0}', provider:{1}, name:'{0} - name', description:'{0} - description' ){{id}}", target, (uint)providerId );
		let createJson = QL().QuerySync( Str::Replace(create, '\'', '"'), executer );
		return { Tests::GetId(createJson) };//{"user":{"id":7}}}
	}
	α createGroup( str target, UserPK executer )ε->GroupPK{
		let create = Ƒ( "mutation createGrouping(  target:'{0}', name:'{0} - name', description:'{0} - description' ){{id}}", target );
		let createJson = QL().QuerySync( Str::Replace(create, '\'', '"'), executer );
		return {AsNumber<GroupPK::Type>( createJson, "id")};
	}
	α columns( sv cols, bool includeDeleted )ε->string{
		return Ƒ( "id name attributes created updated target description {} {}", cols, includeDeleted ? "deleted" : "" );
	}
	α select( sv table, str filter, str cols, UserPK executer )ε->jobject{
		let ql = Ƒ( "{}({}){{ {} }}", table, filter, cols );
		return QL().QuerySync( ql, executer );
	}

	α Tests::Select( sv table, uint id, UserPK executer, sv cols, bool includeDeleted )ε->jobject{
		return select( DB::Names::ToSingular(table), Ƒ("id:{} ", id), columns(cols, includeDeleted), executer );
	}

	α Tests::Select( sv table, str target, UserPK executer, sv cols, bool includeDeleted )ε->jobject{
		return select( DB::Names::ToSingular(table), Ƒ("target:\"{}\" ", target), columns(cols, includeDeleted), executer );
	}

	α Tests::SelectGroup( str target, UserPK executer, bool includeDeleted )ε->jobject{
		let ql = Ƒ( "grouping(target:\"{}\"){{ id name attributes created updated target description {} groupMembers{{id name}} }}", target, includeDeleted ? "deleted" : "" );
		return QL().QuerySync( ql, executer );
	}
	α Tests::SelectPermission( ResourcePK resourcePK, UserPK executer )ε->jobject{
		let ql = Ƒ( "permission( resourceId:{} ){{ id resourceId allowed denied }}", resourcePK );
		return QL().QuerySync( ql, executer );
	}

	α Tests::SelectResource( str target, UserPK executer, bool includeDeleted, SL sl )ε->jobject{
		let ql = Ƒ( "resource( schemaName:\"access\", target:\"{}\", criteria:null ){{ id schemaName allowed denied name attributes created {} updated target description }}", target, includeDeleted ? "deleted" : "" );
		return QL().QuerySync( ql, executer, true, sl );
	}
	α Tests::SelectUser( str target, UserPK executer, bool includeDeleted )->jobject{
		let selectAll = Ƒ( "user(target:\"{}\"){{ id name attributes created updated target description provider {} }}", target, includeDeleted ? "deleted" : "" );
		return QL().QuerySync( selectAll, executer );
	}

	α Tests::Get( str table, str target, UserPK executer, sv cols, bool includeDeleted )ε->jobject{
		auto y = Select( table, target, executer, cols, includeDeleted );
		if( y.empty() ){
			Create( table, target, executer );
			y = Select( table, target, executer, cols, includeDeleted );
		}
		return y;
	}
	optional<UserPK> _root;
	α Tests::GetRoot()ε->UserPK{
		if( _root )
			return *_root;

		auto root = SelectUser( "root", {UserPK::System} );
		if( root.empty() ){
			_root = createUser( "root", (ProviderPK)EProviderType::Google, {UserPK::System} );
			let resourcePermissions = BlockAwait<ResourceLoadAwait,ResourcePermissions>( ResourceLoadAwait(_localQL, {GetTable("acl")->Schema}, {UserPK::System}) );
			for( let& [pk,resource] : resourcePermissions.Resources )
				CreateAcl( *_root, ERights::All, ERights::None, resource.Target, {UserPK::System} );
		}
		else
			_root = UserPK{ GetId(root) };
		return *_root;
	}

	α Tests::GetUser( str target, UserPK executer, bool includeDeleted, ProviderPK provider )ε->jobject{
		if( executer==UserPK{0} )
		executer = GetRoot();
		auto user = SelectUser( target, executer, includeDeleted );
		if( user.empty() ){
			createUser( target, provider, executer );
			user = SelectUser( target, executer, includeDeleted );
		}
		return user;
	}
	α Tests::GetGroup( str target, UserPK executer )ε->jobject{
		auto y = SelectGroup( target, executer, true );
		TRACE( "{}", serialize(y) );
		if( y.empty() ){
			createGroup( target, executer );
			y = SelectGroup( target, executer, false );
		}
		return y;
	}

	α Tests::Purge( str table, uint id, UserPK executer )ε->jvalue{
		let ql = Ƒ( "mutation purge{}(\"id\":{})", Capitalize(table), id );
		let y = QL().QuerySync<jvalue>( ql, executer );
		return y;
	}
	α Tests::PurgeUser( UserPK userId, UserPK executer, SL sl )ε->void{
		let purge = Ƒ( "mutation purgeUser(\"id\":{})", userId.Value );
		let purgeJson = QL().QuerySync<jvalue>( purge, executer, true, sl );
	}
	α Tests::PurgeGroup( GroupPK id, UserPK executer )ε->void{
		let purge = Ƒ( "mutation purgeGrouping(\"id\":{})", id.Value );
		let purgeJson = QL().QuerySync<jvalue>( purge, executer );
	}
	α Tests::Delete( str table, uint id, UserPK executer )ε->jvalue{
		let del = Ƒ( "mutation delete{}( id:{} )", Capitalize(table), id );
		return QL().QuerySync<jvalue>( del, executer );
	}
	α Tests::Restore( str table, uint id, UserPK executer )ε->jvalue{
		let ql = Ƒ( "mutation restore{}( id:{} )", Capitalize(table), id );
		return QL().QuerySync<jvalue>( ql, executer );
	}
	α Tests::TestAdd( str tableName, uint groupPK, vector<uint> members, UserPK executer )->void{
		auto getMemberIds = [&]()->flat_set<uint>{
			let o = Select( tableName, groupPK, executer, {"groupMembers{id}"}, true );
			flat_set<uint> memberIds;
			for( let& member : o.empty() ? jarray{} : AsArray(o, "groupMembers") )
				memberIds.emplace( GetId(AsObject(member)) );
			return memberIds;
		};
		auto memberIds = getMemberIds();
		if( memberIds.find(members.front())==memberIds.end() ){
			Add( *GetTable(tableName), groupPK, members, executer );
			memberIds = getMemberIds();
		}
		for( let member : members )
			ASSERT_TRUE( memberIds.contains(member) ) << "member not found: " << member;
	}
	α Tests::TestRemove( str tableName, uint groupPK, vector<uint> members, UserPK executer )->void{
		Remove( *GetTable(tableName), groupPK, members, executer );
		let o = Select( ToSingular(tableName), groupPK, GetRoot(), {"groupMembers{id}"}, true );
		flat_set<uint> memberIds;
		for( let& member : AsArray(o, "groupMembers") )
			memberIds.emplace( AsNumber<uint>(AsObject(member), "id") );
		for( let member : members )
			ASSERT_TRUE( !memberIds.contains(member) );
	}
}

namespace Jde::Access{
	α Tests::TestCrud( str table, str target, UserPK executer )ε->uint{
		let row = Get( table, target, executer, {}, true );
		let id = GetId( row );
		testUpdateName( table, id, executer, "newName" );
		testDeleteRestore( table, id, executer );
		return id;
	}
	α Tests::TestPurge( str table, uint id, UserPK executer )ε->void{
		Purge( table, id, executer );
 		ASSERT_TRUE( Select(table, id, executer, {}, true).empty() );
	}

	α Tests::TestUnauthCrud( str table, str target, UserPK executer )ε->uint{
		let row = testUnauthGet( table, target, executer, {}, true );
		let id = GetId( row );
		TestUnauthUpdateName( table, id, executer, "newName" );
		TestUnauthDeleteRestore( table, id, executer );
		return id;
	}
	α Tests::TestUnauthAddRemove( str tableName, uint pk, vector<uint> members, UserPK executer )->void{
		let& table = *GetTable( tableName );
		EXPECT_THROW( addRemove("add", table, pk, members, executer), IException );
		EXPECT_THROW( addRemove("remove", table, pk, members, executer), IException );
	}
	α Tests::TestUnauthPurge( str table, uint id, UserPK executer )ε->void{
		EXPECT_THROW( Purge(table, id, executer), IException );
		Purge( table, id, GetRoot() );
	}
}