#include "globals.h"
#include <jde/ql/QLAwait.h>
#include <jde/access/server/awaits/AuthenticateAwait.h>

#define let const auto
namespace Jde::Access::Tests{
	class AuthTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		constexpr static sv OpcServer{"AuthTests::OpcServer1"};
		static ProviderPK OpcProviderId;
	};
	ProviderPK AuthTests::OpcProviderId{};

	α AuthTests::SetUpTestCase()ε->void{
		if( auto o = QL().QuerySync(Ƒ("provider(name:\"{}\"){{id}}", OpcServer), {}, GetRoot()); !o.empty() )
			OpcProviderId = GetId( o );
		else{
			let createQL = Ƒ( "createProvider( target:\"{}\", providerType:{} ){{id}}", OpcServer, underlying(EProviderType::OpcServer) );
			OpcProviderId = GetId( QL().QuerySync(createQL, {}, GetRoot()) );
		}
	}

	α login( str loginName, ProviderPK providerId, string opcServer )ε->UserPK{
		return BlockTAwait<UserPK>( Server::AuthenticateAwait{loginName, providerId, opcServer} );
	}

	TEST_F( AuthTests, Login_Existing ){
		const string user{ "Login_Existing-google" };
		let provider = Access::EProviderType::Google;
		let fetchedUserPK{ GetId( GetUser(user, GetRoot(), true, (ProviderPK)provider) ) };
		let loginUserPK = login( user, underlying(provider), {} );
		ASSERT_EQ( fetchedUserPK, loginUserPK.Value );
		PurgeUser( {fetchedUserPK}, GetRoot() );
	}

	TEST_F( AuthTests, Login_New ){
		const string user{ "Login_New" };
		let userId = login( user, underlying(Access::EProviderType::Google), {} );
		PurgeUser( {userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_Existing_Opc ){
		const string user{ "Login_Existing_Opc" };
		let fetchedUserPK = GetId( GetUser(user, GetRoot(), true, OpcProviderId) );
		let loginUserPK = login( user, OpcProviderId, string{OpcServer} );
		ASSERT_EQ( fetchedUserPK, loginUserPK.Value );
		PurgeUser( {fetchedUserPK}, GetRoot() );
	}

	TEST_F( AuthTests, Login_New_Opc ){
		const string user{ "Login_New_Opc" };
		let userId = login( user, OpcProviderId, string{OpcServer} );
		PurgeUser( {userId}, GetRoot() );
	}
}