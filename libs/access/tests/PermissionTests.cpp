#include "gtest/gtest.h"
#include "globals.h"
#include <jde/framework/str.h>
#include <jde/ql/ql.h>
#include <jde/db/meta/Table.h>
#include "../src/types/Resource.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class PermissionTests : public ::testing::Test{
	};

	α SelectPermission( ResourcePK resourcePK, UserPK userPK )ε->jobject{
		let ql = Ƒ( "query{{ permission( resourceId:{} ){{ id resourceId allowed denied }} }}", resourcePK );
		let qlResult = QL::Query( ql, userPK );
		return Json::FindDefaultObjectPath( qlResult, "data/permission" );
	}
	α GetPermission( ResourcePK resourcePK, ERights allowed, ERights denied, UserPK userPK )ε->jobject{
		auto permission = SelectPermission( resourcePK, userPK );
		if( permission.empty() ){
			let create = Ƒ( "{{ mutation createPermission( input:{{ resourceId:{}, allowed:{}, denied:{} }} ) }}", resourcePK, underlying(allowed), underlying(denied) );
			let createJson = QL::Query( create, userPK );
			permission = SelectPermission( resourcePK, userPK );
		}
		return permission;
	}

	TEST_F( PermissionTests, Crud ){
		auto resourcePK = Json::AsNumber<PermissionPK>( SelectResource("identityGroups", GetRoot()), "id" );
		auto permission = GetPermission( resourcePK, ERights::Read, ERights::Update, GetRoot() );
		ASSERT_TRUE( permission.size() );
		let id = Json::AsNumber<PermissionPK>( permission, "id" );

		let update = Ƒ( "{{ mutation updatePermission( 'id':{}, 'input': {{'allowed': ['Read','Update'], 'denied':['None'] }}) }}", id );
		let updateJson = QL::Query( Str::Replace(update,'\'', '"'), 0 );
		permission = SelectPermission( resourcePK, GetRoot() );
		ASSERT_EQ( Json::AsArray( permission, "allowed" ).size(), 2 );
		ASSERT_EQ( Json::AsArray( permission, "denied" ).size(), 0 );

 		let purge = Ƒ( "{{ mutation purgePermission(\"id\":{}) }}", id );
		let purgeJson = QL::Query( purge, 0 );
		permission = SelectPermission( resourcePK, GetRoot() );
		ASSERT_TRUE( permission.empty() );
	}
}
