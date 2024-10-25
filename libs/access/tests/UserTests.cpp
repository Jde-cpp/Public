#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/db/Database.h>
#include <jde/access/usings.h>
#include "globals.h"

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Test };
	using namespace Json;
	using namespace Tests;

	class UserTests : public ::testing::Test{
	protected:
	};
	std::condition_variable_any cv;
	std::shared_mutex mtx;


	TEST_F( UserTests, Fields ){
		let query = "{ __type(name: \"User\") { fields { name type { name kind ofType{name kind} } } }}";
		let json = QL::Query( query, 0 );
		Trace{ _tags, "{}", serialize(json) };
		let actual = Str::Replace( serialize(json), '"', '\'' );
		let expected = "{'data':{'__type':{'name':'User','fields':[{'name':'id','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'ID','kind':'SCALAR'}}},{'name':'name','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'String','kind':'SCALAR'}}},{'name':'provider','type':{'name':'Provider','kind':'OBJECT'}},{'name':'target','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'String','kind':'SCALAR'}}},{'name':'attributes','type':{'name':'UInt','kind':'SCALAR'}},{'name':'created','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'DateTime','kind':'SCALAR'}}},{'name':'updated','type':{'name':'DateTime','kind':'SCALAR'}},{'name':'deleted','type':{'name':'DateTime','kind':'SCALAR'}},{'name':'description','type':{'name':'String','kind':'SCALAR'}},{'name':'isGroup','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'Boolean','kind':'SCALAR'}}},{'name':'loginName','type':{'name':'String','kind':'SCALAR'}},{'name':'modulus','type':{'name':'String','kind':'SCALAR'}},{'name':'exponent','type':{'name':'UInt','kind':'SCALAR'}}]}}}";
		ASSERT_EQ( actual, expected );
	}

	α SelectUser( string target )->jobject{
		let selectArray = Ƒ( "query{{ user(target:[\"{}\"]){{id loginName provider{{id name}}}} }}", target );
		let selectArrayJson = QL::Query( selectArray, 0 );
		Trace{ _tags, "selectArrayJson={}", serialize(selectArrayJson) };
		return AsObject( selectArrayJson, "data" );
	}

	TEST_F( UserTests, Crud ){
		const string user{ "crud" };
		let existingUser = SelectUser( user );
		auto id = FindNumberPath<UserPK>( existingUser, "user/id" ).value_or( 0 );
		if( id )
			PurgeUser( id );
		id = CreateUser( user );

	//	auto id = 3;
		let selectAll = "query{ users { id name attributes created updated deleted target description provider } }";
		let selectAllJson = QL::Query( selectAll, 0 );
		Trace{ _tags, "{}", serialize(selectAllJson) };

		let readGroups = "query{ users(filter:{isGroup:{ eq:true}}){ id name } }";
		let readGroupsJson = QL::Query( readGroups, 0 );
		Trace{ _tags, "{}", serialize(readGroupsJson) };
		ASSERT_EQ( AsArray(readGroupsJson, "data/users").size(), 0 );

		let read = Ƒ("query{{ user(filter:{{target:{{ eq:\"{}\"}}}}){{ id name attributes created updated deleted target description isGroup provider{{ id name }} }} }}", user );
		let readJson = QL::Query( read, 0 );
		Trace{ _tags, "{}", serialize(readJson) };
		let readId = AsNumber<UserPK>( readJson, "data/user/id" );
		ASSERT_EQ( id, readId );

		let update = Ƒ( "{{ mutation {{ updateUser( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }} }}", readId, "newName" );
		let updateJson = QL::Query( update, 0 );
		Trace{ _tags, "{}", serialize(updateJson) };

		let del = Ƒ( "{{mutation {{ deleteUser(\"id\":{}) }} }}", readId );
		let deleteJson = QL::Query( del, 0 );
		Trace{ _tags, "{}", serialize(deleteJson) };

		PurgeUser( id );
	}

	TEST_F( UserTests, MultipleUsersSelect ){
		UserPK a = CreateUser( "MultipleUsersA" );
		UserPK b = CreateUser( "MultipleUsersB" );
		let q = Ƒ( "query{{ users(id:[{},{}]){{id loginName provider{{id name}}}} }}", a, b );
		auto j = QL::Query( q, 0 );
		Trace{ _tags, "{}", serialize(j) };
		PurgeUser( a );
		PurgeUser( b );
	}
	TEST_F( UserTests, ProvidersSelect ){
		let readGroups = "query{ __type(name: \"Provider\") { enumValues { id name } } }";
		let readGroupsJson = QL::Query( readGroups, 0 );
		Trace{ _tags, "{}", serialize(readGroupsJson) };
		ASSERT_TRUE( AsArray(readGroupsJson, "data/__type/enumValues").size()>0 );
	}
}