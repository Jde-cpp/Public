//ql-review3 #11:  InsertAwait::CreateQuery turns any object-valued arg whose pluralised key names a sibling table into a
//nested INSERT - a real feature (createAcl( identity:{id}, permissionRight:{…} )) - but it authorized nothing, and rights are
//per (schema, resource) with no hierarchy, so Create on groups said nothing about roles.  The nested statement is pushed
//ahead of the outer table's own, so it is also the one that runs first.
//Enforcement here is arranged rather than granted:  a resource is enforced only while it is not deleted, so deleting groups'
//lets any principal create a group, and restoring roles' is what the nested insert has to be refused by.
#include "gtest/gtest.h"
#include <jde/access/Authorize.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	struct NestedInsertAuthorizeTests : ::testing::Test{
		α SetUp()->void override{
			_intruder = UserPK{ GetId(Tests::Get("user", "review11-intruder", GetRoot())) };
			_groups = state( "groups", false );//not enforced: the intruder may create a group.
			_roles = state( "roles", true );  //enforced: the nested role is what must be refused.
		}
		α TearDown()->void override{
			restore( "groups", _groups );
			restore( "roles", _roles );
		}
		//Puts the resource in the wanted state and reports what it was, so TearDown can put it back:  the suite shares one db.
		Ω state( str target, bool enforced )ε->bool{
			let resource = SelectResource( target, GetRoot(), true );
			let wasDeleted = !resource.at( "deleted" ).is_null();
			if( wasDeleted==enforced )//a deleted resource is not enforced.
				wasDeleted ? Restore( "resources", GetId(resource), GetRoot() ) : Delete( "resources", GetId(resource), GetRoot() );
			return wasDeleted;
		}
		Ω restore( str target, bool wasDeleted )ε->void{
			let resource = SelectResource( target, GetRoot(), true );
			if( !resource.at("deleted").is_null() != wasDeleted )
				wasDeleted ? Delete( "resources", GetId(resource), GetRoot() ) : Restore( "resources", GetId(resource), GetRoot() );
		}
		UserPK _intruder;
		bool _groups{};
		bool _roles{};
	};

	//the finding's shape:  a group the intruder may create, carrying a role it may not.
	TEST_F( NestedInsertAuthorizeTests, NestedInsertNeedsCreateOnTheNestedTable ){
		try{
			QL().QuerySync<jvalue>( R"(mutation createGroup( name:"review11-a", target:"review11-a", role:{ name:"evil", target:"evil" } ))", {}, _intruder );
			ADD_FAILURE() << "the nested role was inserted without Create on roles";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("roles"), string::npos ) << e.what();
		}
		EXPECT_TRUE( Select("role", "evil", GetRoot(), "id", true).empty() ) << "the nested statement runs first - it must not have been executed";//Select, not Get: Get creates what it cannot find.
	}

	//the control:  the same mutation without the nested object still goes through, so the refusal above is the nested table's.
	TEST_F( NestedInsertAuthorizeTests, PlainInsertIsUnaffected ){
		EXPECT_NO_THROW( QL().QuerySync<jvalue>(R"(mutation createGroup( name:"review11-b", target:"review11-b" ))", {}, _intruder) );
		let group = Get( "group", "review11-b", GetRoot(), "id", true );
		ASSERT_FALSE( group.empty() );
		Purge( "groups", GetId(group), GetRoot() );
	}
}
