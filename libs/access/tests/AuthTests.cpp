#include "globals.h"
#include <jde/access/access.h>
#include <jde/ql/ql.h>

#define let const auto
namespace Jde::Access::Tests{
//	constexpr ELogTags _tags{ ELogTags::Test };
	using namespace Tests;

	UserPK _userId{};

	class AuthTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		constexpr static sv OpcServer{"AuthTests::OpcServer1"};
		static uint OpcProviderId;
	};
	std::condition_variable_any _cv;
	std::shared_mutex _mtx;
	uint AuthTests::OpcProviderId{};

	α AuthTests::SetUpTestCase()ε->void{
		auto existing = QL::Query( Ƒ("query{{ provider(target:\"{}\"){{id}} }}", OpcServer), GetRoot() );
		if( auto o = Json::FindObject(existing, "provider"); o )
			OpcProviderId = GetId( *o );
		else{
			let createQL = Ƒ( "mutation createProvider( input:{{ target:\"{}\", providerType:{} }} ){{id}}", OpcServer, underlying(EProviderType::OpcServer) );
			OpcProviderId = GetId( Json::AsObject(QL::Query(createQL, GetRoot()), "provider") );
		}
	}

	α Login( str loginName, uint8 providerId, string opcServer )ε->AuthenticateAwait::Task{
		_userId = co_await Authenticate( loginName, providerId, opcServer );
		std::shared_lock l{ _mtx };
		_cv.notify_one();
	}

	TEST_F( AuthTests, Login_Existing ){
		const string user{ "Login_Existing" };
		let provider = Access::EProviderType::Google;
		const UserPK userId{ GetId( GetUser(user, GetRoot(), true, provider) ) };
		Login( user, underlying(provider), {} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId, GetRoot() );
	}

	TEST_F( AuthTests, Login_New ){
		const string user{ "Login_New" };
		Login( user, underlying(Access::EProviderType::Google), {} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		PurgeUser( {_userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_Existing_Opc ){
		const string user{ "Login_Existing_Opc" };
		let userId = GetId( GetUser(user, GetRoot(), true, (EProviderType)OpcProviderId) );
		Login( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		ASSERT_EQ( UserPK{userId}, _userId );
		PurgeUser( {userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_New_Opc ){
		const string user{ "Login_New_Opc" };
		Login( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		PurgeUser( {_userId}, GetRoot() );
	}
}