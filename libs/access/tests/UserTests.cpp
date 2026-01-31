#include "gtest/gtest.h"
#include <jde/ql/QLAwait.h>
#include <jde/access/usings.h>
#include "globals.h"

#define let const auto
namespace Jde::Access{
	using namespace Json;
	using namespace Tests;

	class UserTests : public ::testing::Test{
	protected:
	};
	std::condition_variable_any cv;
	std::shared_mutex mtx;


	TEST_F( UserTests, Fields ){
		let query = "{ __type(name: \"User\") { fields { name type { name kind ofType{name kind} } } }}";
		let json = QL().QuerySync( query, {}, GetRoot() );
		let actual = Str::Replace( serialize(json), '"', '\'' );
		let expected = "{'name':'User','fields':[{'name':'id','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'ID','kind':'SCALAR'}}},{'name':'name','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'String','kind':'SCALAR'}}},{'name':'provider','type':{'name':'Provider','kind':'ENUM'}},{'name':'target','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'String','kind':'SCALAR'}}},{'name':'attributes','type':{'name':'UInt','kind':'SCALAR'}},{'name':'created','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'DateTime','kind':'SCALAR'}}},{'name':'updated','type':{'name':'DateTime','kind':'SCALAR'}},{'name':'deleted','type':{'name':'DateTime','kind':'SCALAR'}},{'name':'description','type':{'name':'String','kind':'SCALAR'}},{'name':'isGroup','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'Boolean','kind':'SCALAR'}}},{'name':'loginName','type':{'name':'String','kind':'SCALAR'}},{'name':'modulus','type':{'name':'String','kind':'SCALAR'}},{'name':'exponent','type':{'name':'UInt','kind':'SCALAR'}}]}";
		ASSERT_EQ( actual, expected );
	}

	TEST_F( UserTests, Crud ){
		const string target{ "crud" };
		let existingUser = GetUser( target, GetRoot() );
		auto id = FindNumberPath<UserPK::Type>( existingUser, "id" ).value_or( 0 );
		ASSERT_NE( id, 0 );

		let update = Ƒ( "mutation updateUser( \"id\":{}, \"name\":\"{}\" )", id, "newName" );
		let updateJson = QL().QuerySync<jvalue>( update, {}, GetRoot() );
		ASSERT_TRUE( AsSV(Tests::GetUser(target, GetRoot()), "name")=="newName" );

		let del = Ƒ( "mutation deleteUser(\"id\":{})", id );
		let deleteJson = QL().QuerySync<jvalue>( del, {}, GetRoot() );
		ASSERT_TRUE( Tests::SelectUser(target, GetRoot()).empty() );
		ASSERT_TRUE( !Tests::GetUser(target, GetRoot(), true).empty() );

		PurgeUser( {id}, GetRoot() );
		ASSERT_TRUE( Tests::SelectUser(target, GetRoot(), true).empty() );
	}

	TEST_F( UserTests, MultipleUsersSelect ){
		let a = GetId( GetUser("MultipleUsersA", GetRoot()) );
		let b = GetId( GetUser("MultipleUsersB", GetRoot()) );
		auto q = "query{ users(id:[$a,$b]){id loginName provider} }";
		auto vars = jobject{ {"a", a}, {"b", b} };
		auto saved = QL().QuerySync<jarray>( move(q), move(vars), GetRoot() );
		ASSERT_EQ( saved.size(), 2 );
		ASSERT_EQ( saved[0].at("loginName").get_string(), "MultipleUsersA" );
		ASSERT_EQ( saved[1].at("loginName").get_string(), "MultipleUsersB" );
		PurgeUser( {a}, GetRoot() );
		PurgeUser( {b}, GetRoot() );
	}
	TEST_F( UserTests, ProvidersSelect ){
		let readGroups = "__type(name: \"Provider\") { enumValues { id name } }";
		let readGroupsJson = QL().QuerySync( readGroups, {}, GetRoot() );
		ASSERT_TRUE( AsArrayPath(readGroupsJson, "enumValues").size()>0 );
	}
}