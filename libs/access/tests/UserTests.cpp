#include "gtest/gtest.h"
#include <jde/ql/GraphQL.h>
#include <jde/db/Database.h>

#define let const auto
namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	class UserTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		constexpr static sv OpcServer{"UserTests::OpcServer1"};
		static uint OpcProviderId;
	};
	uint UserTests::OpcProviderId{};

	α UserTests::SetUpTestCase()->void{
		UM::Configure();

		if( DB::Scaler<uint>(Jde::format("select count(*) from um_providers where target='{}'", OpcServer))==0 )
			DB::Execute( Jde::format("insert into um_providers(provider_type_id, target) values({}, '{}')", (uint)UM::EProviderType::OpcServer, OpcServer), {} );
		OpcProviderId = DB::Scaler<uint>( Jde::format("select id from um_providers where target='{}'", OpcServer) ).value_or(-1);
	}
	std::condition_variable_any cv;
	std::shared_mutex mtx;


	TEST_F( UserTests, Fields ){
		let query = "{ __type(name: \"User\") { fields { name type { name kind ofType{name kind} } } }}";
		let json = UM::Query( query, 0 );
		TRACE( "{}", json.dump() );
		ASSERT_EQ( Str::Replace( json.dump(), '"', '\'' ), "{'data':{'__type':{'fields':[{'name':'id','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'ID'}}},{'name':'name','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'attributes','type':{'kind':0,'name':'UInt'}},{'name':'created','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'DateTime'}}},{'name':'updated','type':{'kind':0,'name':'DateTime'}},{'name':'deleted','type':{'kind':0,'name':'DateTime'}},{'name':'target','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'description','type':{'kind':0,'name':'String'}},{'name':'isGroup','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'Boolean'}}},{'name':'provider','type':{'kind':1,'name':'Provider'}},{'name':'loginName','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'password','type':{'kind':0,'name':'String'}}],'name':'User'}}}" );
	}
	α CreateUser( str name, uint providerId=(uint)UM::EProviderType::Google )->UserPK{
		let create = Jde::format( "{{ mutation {{ createUser(  'input': {{'loginName':'{0}','target':'{0}','provider':{1},'name':'{0} - name','description':'{0} - description'}} ){{id}} }} }}", name, providerId );
		let createJson = UM::Query( Str::Replace(create, '\'', '"'), 0 );
		TRACE( "{}", createJson.dump() );
		return createJson["data"]["user"]["id"].get<UserPK>();//{"data":{"user":{"id":7}}}
	}
	α PurgeUser( UserPK userId )->void{
		let purge = Jde::format( "{{mutation {{ purgeUser(\"id\":{}) }} }}", userId );
		let purgeJson = UM::Query( purge, 0 );
		TRACE( "purgeJson={}", purgeJson.dump() );
	}

	α SelectUser( string target )->json{
		let selectArray = Jde::format( "query{{ user(target:[\"{}\"]){{id loginName provider{{id name}}}} }}", target );
		let selectArrayJson = UM::Query( selectArray, 0 );
		TRACE( "selectArrayJson={}", selectArrayJson.dump() );
		return selectArrayJson["data"];
	}

	TEST_F( UserTests, Crud ){
		const string user{ "crud" };
		let existingUser = SelectUser( user );
		auto id = existingUser.find("user")!=existingUser.end() ? existingUser["user"]["id"].get<UserPK>() : 0;
		if( id )
			PurgeUser( id );
		id = CreateUser( user );

		let selectAll = "query{ users { id name attributes created updated deleted target description provider } }";
		let selectAllJson = UM::Query( selectAll, 0 );
		TRACE( "{}", selectAllJson.dump() );

		let readGroups = "query{ users(filter:{isGroup:{ eq:true}}){ id name } }";
		let readGroupsJson = UM::Query( readGroups, 0 );
		TRACE( "{}", readGroupsJson.dump() );
		ASSERT_EQ( readGroupsJson["data"]["users"].size(), 0 );

		let read = Jde::format("query{{ user(filter:{{target:{{ eq:\"{}\"}}}}){{ id name attributes created updated deleted target description isGroup provider{{ id name }} }} }}", user );
		let readJson = UM::Query( read, 0 );
		TRACE( "{}", readJson.dump() );
		let readId = readJson["data"]["user"]["id"].get<int>();
		ASSERT_EQ( id, readId );

		let update = Jde::format( "{{ mutation {{ updateUser( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }} }}", readId, "newName" );
		let updateJson = UM::Query( update, 0 );
		TRACE( "{}", updateJson.dump() );

		let del = Jde::format( "{{mutation {{ deleteUser(\"id\":{}) }} }}", readId );
		let deleteJson = UM::Query( del, 0 );
		TRACE( "{}", deleteJson.dump() );

		PurgeUser( id );
	}

	TEST_F( UserTests, MultipleUsersSelect ){
		UserPK a = CreateUser( "MultipleUsersA" );
		UserPK b = CreateUser( "MultipleUsersB" );
		let q = Jde::format( "query{{ users(id:[{},{}]){{id loginName provider{{id name}}}} }}", a, b );
		auto j = UM::Query( q, 0 );
		TRACE( "{}", j.dump() );
		PurgeUser( a );
		PurgeUser( b );
	}
	TEST_F( UserTests, ProvidersSelect ){
		let readGroups = "query{ __type(name: \"Provider\") { enumValues { id name } } }";
		let readGroupsJson = UM::Query( readGroups, 0 );
		TRACE( "{}", readGroupsJson.dump() );
		ASSERT_TRUE( readGroupsJson["data"]["__type"]["enumValues"].size()>0 );
	}

	uint _userId{};
	α Login( str loginName, uint providerId, string opcServer )->Task{
		up<UserPK> pUserId = ( co_await UM::Login( loginName, providerId, opcServer) ).UP<UserPK>();
		_userId = *pUserId;
		std::shared_lock l{ mtx };
		cv.notify_one();
	}

	TEST_F( UserTests, Login_Existing ){
		const string user{ "Login_Existing" };
		let provider = UM::EProviderType::Google;
		let userId = CreateUser( user, (uint)provider );
		Jde::Login( user, (uint)provider, {} );
		std::shared_lock l{ mtx };
		cv.wait( l );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId );
	}

	TEST_F( UserTests, Login_New ){
		const string user{ "Login_New" };
		let provider = UM::EProviderType::Google;
		Jde::Login( user, (uint)provider, {} );
		std::shared_lock l{ mtx };
		cv.wait( l );
		PurgeUser( _userId );
	}

TEST_F( UserTests, Login_Existing_Opc ){
		const string user{ "Login_Existing_Opc" };
		let userId = CreateUser( user, (uint)OpcProviderId );
		Jde::Login( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ mtx };
		cv.wait( l );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId );
	}

	TEST_F( UserTests, Login_New_Opc ){
		const string user{ "Login_New_Opc" };
		Jde::Login( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ mtx };
		cv.wait( l );
		PurgeUser( _userId );
	}
}