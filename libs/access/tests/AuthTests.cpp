#include "globals.h"
#include <jde/access/access.h>
#include <jde/ql/ql.h>

#define let const auto
namespace Jde::Access::Tests{
	UserPK _userId{};

	class AuthTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		constexpr static sv OpcServer{"AuthTests::OpcServer1"};
		static ProviderPK OpcProviderId;
	};
	std::condition_variable_any _cv;
	std::shared_mutex _mtx;
	ProviderPK AuthTests::OpcProviderId{};

	α AuthTests::SetUpTestCase()ε->void{
	if( auto o = QL::QueryObject(Ƒ("provider(target:\"{}\"){{id}}", OpcServer), GetRoot()); !o.empty() )
			OpcProviderId = GetId( o );
		else{
			let createQL = Ƒ( "createProvider( target:\"{}\", providerType:{} ){{id}}", OpcServer, underlying(EProviderType::OpcServer) );
			OpcProviderId = GetId( QL::QueryObject(createQL, GetRoot()) );
		}
	}

	α login( str loginName, ProviderPK providerId, string opcServer )ε->AuthenticateAwait::Task{
		_userId = co_await Authenticate( loginName, providerId, opcServer );
		std::shared_lock l{ _mtx };
		_cv.notify_one();
	}

	TEST_F( AuthTests, Login_Existing ){
		const string user{ "Login_Existing" };
		let provider = Access::EProviderType::Google;
		const UserPK userId{ GetId( GetUser(user, GetRoot(), true, (ProviderPK)provider) ) };
		login( user, underlying(provider), {} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId, GetRoot() );
	}

	TEST_F( AuthTests, Login_New ){
		const string user{ "Login_New" };
		login( user, underlying(Access::EProviderType::Google), {} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		PurgeUser( {_userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_Existing_Opc ){
		const string user{ "Login_Existing_Opc" };
		let userId = GetId( GetUser(user, GetRoot(), true, OpcProviderId) );
		login( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		ASSERT_EQ( UserPK{userId}, _userId );
		PurgeUser( {userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_New_Opc ){
		const string user{ "Login_New_Opc" };
		login( user, OpcProviderId, string{OpcServer} );
		std::shared_lock l{ _mtx };
		_cv.wait( l );
		PurgeUser( {_userId}, GetRoot() );
	}
}