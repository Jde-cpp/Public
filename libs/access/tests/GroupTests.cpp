#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/ql/types/Introspection.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/framework/str.h>
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class GroupTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};

	α GroupTests::SetUpTestCase()->void{
	}
	Ω IsMember( str target, GroupPK child, UserPK executer )ε->bool{
		let group = SelectGroup( target, executer, true );
		if( auto members = FindArray(group, "groupMembers"); members ){
			for( let& member : *members ){
				if( GetId(AsObject(member))==child.Value )
					return true;
			}
		}
		return false;
	}
	TEST_F( GroupTests, Fields ){
		//const QL::TableQL ql{ "" };
		let query = "{ __type(name: \"Grouping\") { fields { name type { name kind ofType{name kind ofType{name kind ofType{name kind}}} } } } }";
		let actual = QL().QuerySync( query, GetRoot() );
		auto obj = Json::ReadJsonNet( Ƒ("{}/Public/libs/access/config/access-ql.jsonnet", OSApp::EnvironmentVariable("JDE_DIR").value_or("./")) );
		QL::Introspection intro{ move(obj) };
		QL::RequestQL request = QL::Parse( query, Schemas() );
		jobject expected = intro.Find("Grouping")->ToJson( request.TableQLs()[0].Tables[0] );
		ASSERT_EQ( serialize(actual), serialize(expected) );
	}

	TEST_F( GroupTests, Crud ){
		let group = GetGroup( "groupTest", GetRoot() );
		let id = GetId( group );

 		let update = Ƒ( "mutation updateGrouping( \"id\":{}, \"name\":\"{}\" )", id, "newName" );
 		let updateJson = QL().QuerySync<jvalue>( update, GetRoot() );
		ASSERT_TRUE( AsSV(SelectGroup("groupTest", GetRoot()), "name")=="newName" );

 		let del = Ƒ( "{{mutation deleteGrouping(\"id\":{}) }}", id );
 		let deleteJson = QL().QuerySync<jvalue>( del, GetRoot() );
		ASSERT_TRUE( SelectGroup("groupTest", GetRoot()).empty() );
		ASSERT_TRUE( !SelectGroup("groupTest", GetRoot(), true).empty() );

 		PurgeGroup( {id}, GetRoot() );
 		ASSERT_TRUE( SelectGroup("groupTest", GetRoot(), true).empty() );
	}
	TEST_F( GroupTests, AddRemove ){
		let root = GetRoot();
		const GroupPK hrManagers{ GetId(GetGroup("HR-Managers", root)) };
		const UserPK manager{ GetId(GetUser("manager", root)) };
		RemoveFromGroup( hrManagers, {manager}, root );
		AddToGroup( hrManagers, {manager}, root );
		constexpr sv ql = "grouping(id:{}){{ groupMembers{{id name}} }}";
		ASSERT_EQ( Json::AsArray(QL().QuerySync( Ƒ(ql, hrManagers.Value), root), "groupMembers").size(), 1 );

		const GroupPK hr{ GetId( GetGroup("HR", root) ) };
		const UserPK associate{ GetId( GetUser("associate", root) ) };
		RemoveFromGroup( hr, {hrManagers, associate}, root );
		AddToGroup( {hr}, {hrManagers, associate}, root );
		let members = QL().QuerySync( Ƒ(ql, hr.Value), root );
		let array = Json::AsArrayPath( members, "groupMembers" );
		ASSERT_EQ( array.size(), 2 );

		constexpr sv userQL = "user(id:{}){{ groupings{{id name}} }}";
		ASSERT_EQ( Json::AsArrayPath(QL().QuerySync(Ƒ(userQL, manager.Value), root), "groupings" ).size(), 1 );

		RemoveFromGroup( hr, {hrManagers, associate}, root );
		RemoveFromGroup( hrManagers, {manager}, root );
		PurgeGroup( hr, root );
		PurgeGroup( hrManagers, root );
		PurgeUser( associate, root );
		PurgeUser( manager, root );
	}

	TEST_F( GroupTests, Recursion ){
		const GroupPK groupA{ GetId(GetGroup("groupA", GetRoot())) };
		const GroupPK groupB{ GetId(GetGroup("groupB", GetRoot())) };
		if( !IsMember( "groupA", groupB, GetRoot()) )
			AddToGroup( groupA, {groupB}, GetRoot() );
		const GroupPK groupC{ GetId(GetGroup("groupC", GetRoot())) };
		if( !IsMember( "groupB", groupC, GetRoot()) )
			AddToGroup( groupB, {groupC}, GetRoot() );

		const GroupPK groupD{ GetId(GetGroup("groupD", GetRoot())) };
		EXPECT_THROW( AddToGroup( groupD, {groupD}, GetRoot() ), IException );
		if( !IsMember( "groupC", groupD, GetRoot()) )
			AddToGroup( groupC, {groupD}, GetRoot() );
		EXPECT_THROW( AddToGroup( groupD, {groupA}, GetRoot() ), IException );
		//TODO test implement deleted members.
	}
}