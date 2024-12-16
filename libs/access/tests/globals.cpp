#include "globals.h"
#include <jde/framework/str.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>

#define let const auto

namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::Access{
	using namespace DB::Names;
	constexpr ELogTags _tags{ ELogTags::Test | ELogTags::Pedantic };
	static sp<DB::AppSchema> _schema;

namespace Tests{
	Ω testUnauthGet( str table, str target, UserPK executer, sv cols, bool includeDeleted )ε->jobject{
		auto y = Select( table, target, GetRoot(), cols, includeDeleted );
		if( y.empty() ){
			EXPECT_THROW( Create(table, target, executer), IException );
			Create( table, target, GetRoot() );
			EXPECT_THROW( Select(table, target, executer, cols, includeDeleted), IException );
			y = Select( table, target, GetRoot(), cols, includeDeleted );
		}
		return y;
	}

	Ω updateNameQL( str table, uint id, sv updatedName )ε->string{
		return Str::Replace( Ƒ("mutation update{}( 'id':{}, 'input': {{'name':'{}'}} ) }}", Capitalize(table), id, updatedName), '\'', '"' );
	}
	Ω testUpdateName( str table, uint id, UserPK userPK, sv updatedName )ε->void{
 		let updateJson = QL::Query( updateNameQL(table, id, updatedName), userPK );
		ASSERT_TRUE( Json::AsSV(Select(table,id, GetRoot(), {}, true), "name")==updatedName );
	}

	Ω deleteQL( str table, uint id )ι->string{ return Ƒ( "mutation delete{}( \"id\":{} )", Capitalize(table), id ); }
	Ω restoreQL( str table, uint id )ι->string{ return Ƒ( "mutation restore{}( \"id\":{} )", Capitalize(table), id ); }
	Ω testDeleteRestore( str table, uint id, UserPK userPK )ε->void{
		let del = Ƒ( "mutation delete{}(\"id\":{})", Capitalize(table), id );
		let deleteJson = QL::Query( deleteQL(table, id), userPK );
		ASSERT_TRUE( Select(table, id, userPK).empty() );
		ASSERT_FALSE( Select(table, id, userPK, {}, true).empty() );

 		let restoreJson = QL::Query( restoreQL(table, id), userPK );
		ASSERT_FALSE( Select(table, id, userPK).empty() );
	}
	Ω addRemoveQL( sv op, const DB::Table& table, uint pk, vector<uint> members )ε->string{
		let& map = *table.Map;
		let parentTable = map.Parent->Table;
		let parentTableName = Capitalize( parentTable->Name );
		let memberString = members.size()==1 ? Ƒ( "{}", members[0] ) : '['+Str::Join( members )+']';
		return Ƒ( "{{ mutation {}{}( \"id\":{}, \"{}\":{} ) }}", op, parentTable->Name, pk, ToJson(map.Child->Name), memberString );
	}
	Ω addRemove( sv op, const DB::Table& table, uint pk, vector<uint> members, UserPK userPK )ε->jobject{
		return QL::QueryObject( addRemoveQL(op, table, pk, members), userPK );
	}
}
	α Tests::TestUnauthUpdateName( str table, uint id, UserPK userPK, sv updatedName )ε->void{
 		EXPECT_THROW( QL::Query(updateNameQL(table, id, updatedName), userPK), IException );
	}
	α Tests::TestUnauthDeleteRestore( str table, uint id, UserPK userPK )ε->void{
		EXPECT_THROW( QL::Query(deleteQL(table,id), userPK), IException );
		EXPECT_THROW( QL::Query(restoreQL(table,id), userPK), IException );
	}

	α Tests::SetSchema( sp<DB::AppSchema> schema )ι->void{_schema = schema;}
	α Tests::DS()ι->DB::IDataSource&{ return *_schema->DS(); }
	α Tests::GetTable( str name )ι->sp<DB::Table>{ return _schema->GetTablePtr( FromJson(name) ); }
	using namespace Json;

	α Tests::Add( const DB::Table& table, uint groupPK, vector<uint> members, UserPK userPK )ε->void{
		addRemove( "add", table, groupPK, members, userPK );
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
	α Tests::AddToGroup( GroupPK id, vector<IdentityPK> members, UserPK userPK )ε->void{
		let ql =	Ƒ( "{{ mutation addIdentityGroup( \"id\":{}, \"memberId\":{} ) }}", id.Value, memberString(members) );
		let addJson = QL::Query( ql, userPK );
	}

	α Tests::Remove( const DB::Table& table, uint groupPK, vector<uint> members, UserPK userPK )ε->void{
		addRemove( "remove", table, groupPK, members, userPK );
	}
	α Tests::RemoveFromGroup( GroupPK id, vector<IdentityPK> members, UserPK userPK )ε->void{
		let ql = Ƒ( "{{ mutation removeIdentityGroup( \"id\":{}, \"memberId\":{} ) }}", id.Value, memberString(members) );
		let removeJson = QL::Query( ql, userPK );
	}


	α Tests::Create( str table, sv target, UserPK userPK, str input )ε->uint{
		let create = Ƒ( "{{ mutation create{0}(  'input': {{'target':'{1}','name':'{1} - name','description':'{1} - description' {2} }} ){{id}} }}", Capitalize(table), target, input.size() ? ","s+input : "" );
		let createJson = FindDefaultObject( QL::Query(Str::Replace(create, '\'', '"'), userPK), table );
		return GetId( createJson );//{"data":{"user":{"id":7}}}
	}

	Ω createUser( str target, EProviderType providerId, UserPK userPK )ε->UserPK{
		let create = Ƒ( "{{ mutation createUser(  'input': {{'loginName':'{0}','target':'{0}','provider':{1},'name':'{0} - name','description':'{0} - description'}} ){{id}} }}", target, (uint)providerId );
		let createJson = QL::QueryObject( Str::Replace(create, '\'', '"'), userPK );
		return { AsNumber<UserPK::Type>( createJson, "user/id") };//{"data":{"user":{"id":7}}}
	}
	α createGroup( str target, UserPK userPK )ε->GroupPK{
		let create = Ƒ( "{{ mutation createIdentityGroup(  'input': {{'target':'{0}','name':'{0} - name','description':'{0} - description'}} ){{id}} }}", target );
		let createJson = QL::QueryObject( Str::Replace(create, '\'', '"'), userPK );
		return {AsNumber<GroupPK::Type>( createJson, "identityGroup/id")};
	}
	α columns( sv cols, bool includeDeleted )ε->string{
		return Ƒ( "id name attributes created updated target description {} {}", cols, includeDeleted ? "deleted" : "" );
	}
	α select( sv table, str filter, str cols, UserPK userPK )ε->jobject{
		let ql = Ƒ( "{}({}){{ {} }}", table, filter, cols );
		return QL::QueryObject( ql, userPK );
	}

	α Tests::Select( sv table, uint id, UserPK userPK, sv cols, bool includeDeleted )ε->jobject{
		return select( DB::Names::ToSingular(table), Ƒ("id:{} ", id), columns(cols, includeDeleted), userPK );
	}

	α Tests::Select( sv table, str target, UserPK userPK, sv cols, bool includeDeleted )ε->jobject{
		return select( DB::Names::ToSingular(table), Ƒ("target:\"{}\" ", target), columns(cols, includeDeleted), userPK );
	}

	α Tests::SelectGroup( str target, UserPK userPK, bool includeDeleted )ε->jobject{
		let ql = Ƒ( "query{{ identityGroup(target:\"{}\"){{ id name attributes created updated target description {} members{{id name}} }} }}", target, includeDeleted ? "deleted" : "" );
		let y = QL::Query( ql, userPK );
		return FindDefaultObject( y, "identityGroup" );
	}
	α Tests::SelectPermission( ResourcePK resourcePK, UserPK userPK )ε->jobject{
		let ql = Ƒ( "query{{ permission( resourceId:{} ){{ id resourceId allowed denied }} }}", resourcePK );
		let qlResult = QL::Query( ql, userPK );
		return Json::FindDefaultObject( qlResult, "permission" );
	}

	α Tests::SelectResource( str target, UserPK userPK, bool includeDeleted )ε->jobject{
		let ql = Ƒ( "query{{ resource( schemaName:\"access\", target:\"{}\", criteria:null ){{ id schemaName allowed denied name attributes created {} updated target description }} }}", target, includeDeleted ? "deleted" : "" );
		let y = QL::Query( ql, userPK );
		Trace{ _tags, "{}", serialize(y) };
		return FindDefaultObject( y, "resource" );
	}
	α Tests::SelectUser( str target, UserPK userPK, bool includeDeleted )->jobject{
		let selectAll = Ƒ( "query{{ user(target:\"{}\"){{ id name attributes created updated target description provider {}}} }}", target, includeDeleted ? "deleted" : "" );
		let selectAllJson = QL::Query( selectAll, userPK );
		return FindDefaultObject( selectAllJson, "user" );
	}

	α Tests::Get( str table, str target, UserPK userPK, sv cols, bool includeDeleted )ε->jobject{
		auto y = Select( table, target, userPK, cols, includeDeleted );
		if( y.empty() ){
			Create( table, target, userPK );
			y = Select( table, target, userPK, cols, includeDeleted );
		}
		return y;
	}
	optional<UserPK> _root;
	α Tests::GetRoot()ε->UserPK{
		if( _root )
			return *_root;

		auto root = SelectUser( "root", {0} );
		_root = root.empty()
			? createUser( "root", EProviderType::Google, {0} )
			: UserPK{ GetId(root) };
		return *_root;
	}

	α Tests::GetUser( str target, UserPK userPK, bool includeDeleted, EProviderType provider )ε->jobject{
		if( userPK==UserPK{0} )
			userPK = GetRoot();
		auto user = SelectUser( target, userPK, includeDeleted );
		if( user.empty() ){
			createUser( target, provider, userPK );
			user = SelectUser( target, userPK, includeDeleted );
		}
		return user;
	}
	α Tests::GetGroup( str target, UserPK userPK )ε->jobject{
		auto y = SelectGroup( target, userPK, true );
		Trace{ _tags, "{}", serialize(y) };
		if( y.empty() ){
			createGroup( target, userPK );
			y = SelectGroup( target, userPK, false );
		}
		return y;
	}

	α Tests::Purge( str table, uint id, UserPK userPK )ε->jobject{
		let ql = Ƒ( "mutation purge{}(\"id\":{})", Capitalize(table), id );
		let y = QL::QueryObject( ql, userPK );
		return y;
	}
	α Tests::PurgeUser( UserPK userId, UserPK userPK )ε->void{
		let purge = Ƒ( "mutation purgeUser(\"id\":{})", userId.Value );
		let purgeJson = QL::Query( purge, userPK );
	}
	α Tests::PurgeGroup( GroupPK id, UserPK userPK )ε->void{
		let purge = Ƒ( "mutation purgeIdentityGroup(\"id\":{})", id.Value );
		let purgeJson = QL::Query( purge, userPK );
	}
	α Tests::Delete( str table, uint id, UserPK userPK )ε->jobject{
		let del = Ƒ( "mutation delete{}( \"id\":{} )", Capitalize(table), id );
		return QL::QueryObject( del, userPK );
	}
	α Tests::Restore( str table, uint id, UserPK userPK )ε->jobject{
		let ql = Ƒ( "mutation restore{}( \"id\":{} )", Capitalize(table), id );
		return QL::QueryObject( ql, userPK );
	}
	α Tests::TestAdd( str tableName, uint groupPK, vector<uint> members, UserPK userPK )->void{
		Add( *GetTable(tableName), groupPK, members, userPK );
		let o = Select( ToSingular(tableName), groupPK, userPK, {"members{id}"}, true );
		flat_set<uint> memberIds;
		for( let& member : AsArray(o, "members") )
			memberIds.emplace( GetId(AsObject(member)) );
		for( let member : members )
			ASSERT_TRUE( memberIds.contains(member) ) << "member not found: " << member;
	}
	α Tests::TestRemove( str tableName, uint groupPK, vector<uint> members, UserPK userPK )->void{
		Remove( *GetTable(tableName), groupPK, members, userPK );
		let o = Select( ToSingular(tableName), groupPK, GetRoot(), {"members{id}"}, true );
		flat_set<uint> memberIds;
		for( let& member : AsArray(o, "members") )
			memberIds.emplace( AsNumber<uint>(AsObject(member), "id") );
		for( let member : members )
			ASSERT_TRUE( !memberIds.contains(member) );
	}
}

namespace Jde::Access{
	α Tests::TestCrud( str table, str target, UserPK userPK )ε->uint{
		let row = Get( table, target, userPK, {}, true );
		let id = GetId( row );
		testUpdateName( table, id, userPK, "newName" );
		testDeleteRestore( table, id, userPK );
		return id;
	}
	α Tests::TestPurge( str table, uint id, UserPK userPK )ε->void{
		Purge( table, id, userPK );
 		ASSERT_TRUE( Select(table, id, userPK, {}, true).empty() );
	}

	α Tests::TestUnauthCrud( str table, str target, UserPK executer )ε->uint{
		let row = testUnauthGet( table, target, executer, {}, true );
		let id = GetId( row );
		TestUnauthUpdateName( table, id, executer, "newName" );
		TestUnauthDeleteRestore( table, id, executer );
		return id;
	}
	α Tests::TestUnauthAddRemove( str tableName, uint pk, vector<uint> members, UserPK userPK )->void{
		let& table = *GetTable( tableName );
		EXPECT_THROW( addRemove("add", table, pk, members, userPK), IException );
		EXPECT_THROW( addRemove("remove", table, pk, members, userPK), IException );
	}
	α Tests::TestUnauthPurge( str table, uint id, UserPK userPK )ε->void{
		EXPECT_THROW( Purge(table, id, userPK), IException );
		Purge( table, id, GetRoot() );
	}
}