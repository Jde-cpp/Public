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

	α Tests::SetSchema( sp<DB::AppSchema> schema )ι->void{_schema = schema;}
	α Tests::DS()ι->DB::IDataSource&{ return *_schema->DS(); }
	α Tests::GetTable( str name )ι->sp<DB::Table>{ return _schema->GetTablePtr( FromJson(name) ); }
	using namespace Json;
	α addRemove( sv op, const DB::Table& groupTable, uint groupId, vector<uint> members, UserPK userPK )ε->jobject{
		let& map = *groupTable.Map;
		let parentTable = map.Parent->Table;
		let parentTableName = Capitalize( parentTable->Name );
		let memberString = members.size()==1 ? Ƒ( "{}", members[0] ) : '['+Str::Join( members )+']';
		let ql = Ƒ( "{{ mutation {}{}( \"id\":{}, \"{}\":{} ) }}", op, parentTable->Name, groupId, ToJson(map.Child->Name), memberString );
		let y = QL::Query( ql, userPK );
		return y;
	}
	α Tests::Add( const DB::Table& table, uint groupId, vector<uint> members, UserPK userPK )ε->void{
		addRemove( "add", table, groupId, members, userPK );
	}

	α Tests::AddToGroup( GroupPK id, vector<IdentityPK> members, UserPK userPK )ε->void{
		let add = members.size()==1
			? Ƒ( "{{ mutation addIdentityGroup( \"id\":{}, \"memberId\":{} ) }}", id, members[0] )
			: Ƒ( "{{ mutation addIdentityGroup( \"id\":{}, \"memberId\":[{}] ) }}", id, Str::Join(members) );
		let addJson = QL::Query( add, userPK );
	}

	α Tests::Remove( const DB::Table& table, uint groupId, vector<uint> members, UserPK userPK )ε->void{
		addRemove( "remove", table, groupId, members, userPK );
	}
	α Tests::RemoveFromGroup( GroupPK id, vector<IdentityPK> members, UserPK userPK )ε->void{
		let remove = members.size()==1
			? Ƒ( "{{ mutation removeIdentityGroup( \"id\":{}, \"memberId\":{} ) }}", id, members[0] )
			: Ƒ( "{{ mutation removeIdentityGroup( \"id\":{}, \"memberId\":[{}] ) }}", id, Str::Join(members) );
		let removeJson = QL::Query( remove, userPK );
	}


	α Tests::Create( str table, sv target, UserPK userPK, str input )ε->uint{
		let create = Ƒ( "{{ mutation create{0}(  'input': {{'target':'{1}','name':'{1} - name','description':'{1} - description' {2} }} ){{id}} }}", Capitalize(table), target, input.size() ? ","s+input : "" );
		let createJson = QL::Query( Str::Replace(create, '\'', '"'), userPK );
		return AsNumber<uint>( createJson, Ƒ("data/{}/id",ToJson(table)) );//{"data":{"user":{"id":7}}}
	}

	α CreateUser( str target, uint providerId, UserPK userPK )ε->UserPK{
		let create = Ƒ( "{{ mutation createUser(  'input': {{'loginName':'{0}','target':'{0}','provider':{1},'name':'{0} - name','description':'{0} - description'}} ){{id}} }}", target, providerId );
		let createJson = QL::Query( Str::Replace(create, '\'', '"'), userPK );
		return AsNumber<UserPK>( createJson, "data/user/id" );//{"data":{"user":{"id":7}}}
	}
	α CreateGroup( str target, UserPK userPK )ε->IdentityPK{
		let create = Ƒ( "{{ mutation createIdentityGroup(  'input': {{'target':'{0}','name':'{0} - name','description':'{0} - description'}} ){{id}} }}", target );
		let createJson = QL::Query( Str::Replace(create, '\'', '"'), userPK );
		return AsNumber<UserPK>( createJson, "data/identityGroup/id" );
	}
	α columns( sv cols, bool includeDeleted )ε->string{
		return Ƒ( "id name attributes created updated target description {} {}", cols, includeDeleted ? "deleted" : "" );
	}
	α select( sv table, str filter, str cols, UserPK userPK )ε->jobject{
		let ql = Ƒ( "query{{ {}({}){{ {} }} }}", table, filter, cols );
		let y = QL::Query( ql, userPK );
		return FindDefaultObjectPath( y, "data/"+string{table} );
	}

	α Tests::Select( sv table, uint id, UserPK userPK, sv cols, bool includeDeleted )ε->jobject{
		return select( table, Ƒ("id:{} ", id), columns(cols, includeDeleted), userPK );
	}

	α Tests::Select( sv table, str target, UserPK userPK, sv cols, bool includeDeleted )ε->jobject{
		return select( table, Ƒ("target:\"{}\" ", target), columns(cols, includeDeleted), userPK );
	}

	α Tests::SelectGroup( str target, UserPK userPK, bool includeDeleted )ε->jobject{
		let ql = Ƒ( "query{{ identityGroup(target:\"{}\"){{ id name attributes created updated target description {} members{{id name}} }} }}", target, includeDeleted ? "deleted" : "" );
		let y = QL::Query( ql, userPK );
		return FindDefaultObjectPath( y, "data/identityGroup" );
	}
	α Tests::SelectResource( str target, UserPK userPK, bool includeDeleted )ε->jobject{
		let ql = Ƒ( "query{{ resource( schemaName:\"access\", target:\"{}\", criteria:null ){{ id schemaName rights name attributes created {} updated target description }} }}", target, includeDeleted ? "deleted" : "" );
		let y = QL::Query( ql, userPK );
		return FindDefaultObjectPath( y, "data/resource" );
	}
	α Tests::SelectUser( str target, UserPK userPK, bool includeDeleted )->jobject{
		let selectAll = Ƒ( "query{{ user(target:\"{}\"){{ id name attributes created updated target description provider {}}} }}", target, includeDeleted ? "deleted" : "" );
		let selectAllJson = QL::Query( selectAll, userPK );
		return FindDefaultObjectPath( selectAllJson, "data/user" );
	}

	α Tests::Get( str table, str target, UserPK userPK, sv cols, bool includeDeleted )ε->jobject{
		auto y = Select( table, target, userPK, cols, includeDeleted );
		if( y.empty() ){
			Create( table, target, userPK );
			y = Select( table, target, userPK, cols, includeDeleted );
		}
		//Trace{ _tags, "{}", serialize(y) };
		return y;
	}
	optional<UserPK> _root;
	α Tests::GetRoot()ε->UserPK{
		if( _root )
			return *_root;

		auto root = SelectUser( "root", 0 );
		_root = root.empty()
			? GetId(CreateUser( "root", 0, 0 ) )
			: GetId(root);
		return *_root;
	}

	α Tests::GetUser( str target, uint providerId, UserPK userPK, bool includeDeleted )ε->jobject{
		if( userPK==0 )
			userPK = GetRoot();
		auto user = SelectUser( target, userPK, true );
		if( user.empty() ){
			CreateUser( target, providerId, userPK );
			user = SelectUser( target, userPK );
		}
		return user;
	}
	α Tests::GetGroup( str target, UserPK userPK, bool includeDeleted )ε->jobject{
		auto y = SelectGroup( target, userPK, includeDeleted );
		if( y.empty() ){
			CreateGroup( target, userPK );
			y = SelectGroup( target, userPK, includeDeleted );
		}
		return y;
	}

	α Tests::Purge( str table, UserPK userPK, uint id )ε->jobject{
		let ql = Ƒ( "mutation purge{}(\"id\":{})", Capitalize(table), id );
		let y = QL::Query( ql, userPK );
		return y;
	}
	α Tests::PurgeUser( UserPK userId, UserPK userPK )ε->void{
		let purge = Ƒ( "mutation purgeUser(\"id\":{})", userId );
		let purgeJson = QL::Query( purge, userPK );
	}
	α Tests::PurgeGroup( GroupPK id, UserPK userPK )ε->void{
		let purge = Ƒ( "mutation purgeIdentityGroup(\"id\":{})", id );
		let purgeJson = QL::Query( purge, userPK );
	}
	α Tests::Delete( str table, uint id, UserPK userPK )ε->jobject{
		let del = Ƒ( "mutation delete{}( \"id\":{} )", Capitalize(table), id );
		return QL::Query( del, userPK );
	}

	α Tests::TestAdd( str tableName, uint groupId, vector<uint> members, UserPK userPK )->void{
		Add( *GetTable(tableName), groupId, members, userPK );
		let o = Select( ToSingular(tableName), groupId, GetRoot(), {"members{id}"}, userPK );
		flat_set<uint> memberIds;
		for( let& member : AsArray(o, "members") )
			memberIds.emplace( AsNumber<uint>(member) );
		for( let member : members )
			ASSERT_TRUE( memberIds.contains(member) );
	}
	α Tests::TestRemove( str tableName, uint groupId, vector<uint> members, UserPK userPK )->void{
		Remove( *GetTable(tableName), groupId, members, userPK );
		let o = Select( ToSingular(tableName), groupId, GetRoot(), {"members{id}"}, userPK );
		flat_set<uint> memberIds;
		for( let& member : AsArray(o, "members") )
			memberIds.emplace( AsNumber<uint>(member) );
		for( let member : members )
			ASSERT_TRUE( !memberIds.contains(member) );
	}

	α Tests::TestCrud( str table, str target, UserPK userPK )ε->uint{
		let row = Get( table, target, userPK );
		let id = GetId( row );
		TestUpdateName( table, id, userPK );
		TestDelete( table, id, userPK );
		return id;
	}
	α Tests::TestDelete( str table, uint id, UserPK userPK )ε->void{
		let del = Ƒ( "mutation delete{}(\"id\":{})", Capitalize(table), id );
		let deleteJson = QL::Query( del, userPK );
		ASSERT_TRUE( Select(table, id, userPK).empty() );
		ASSERT_FALSE( Select(table, id, userPK, {}, true).empty() );

 		let restore = Ƒ( "mutation restore{}(\"id\":{})", table, id );
 		let restoreJson = QL::Query( restore, userPK );
		ASSERT_FALSE( Select(table, id, userPK).empty() );
	}
	α Tests::TestPurge( str table, uint id, UserPK userPK )ε->void{
		Purge( table, id, userPK );
 		ASSERT_TRUE( Select(table, id, userPK, {}, true).empty() );
	}
	α Tests::TestUpdateName( str table, uint id, UserPK userPK, sv update )ε->void{
 		let ql = Ƒ( "mutation update{}( 'id':{}, 'input': {{'name':'{}'}} ) }}", Capitalize(table), id, update );
 		let updateJson = QL::Query( Str::Replace(ql, '\'', '"'), userPK );
		ASSERT_TRUE( Json::AsSV(Select(table,id, GetRoot(), {}, true), "name")==update );
	}
}