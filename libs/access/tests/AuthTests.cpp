#include "globals.h"
#include <jde/ql/QLAwait.h>
#include <jde/access/server/awaits/AuthenticateAwait.h>

#define let const auto
namespace Jde::Access::Tests{
	UserPK _userId{};

	class AuthTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		constexpr static sv OpcServer{"AuthTests::OpcServer1"};
		static ProviderPK OpcProviderId;
	};
	ProviderPK AuthTests::OpcProviderId{};

	α AuthTests::SetUpTestCase()ε->void{
	if( auto o = QL().QuerySync(Ƒ("provider(target:\"{}\"){{id}}", OpcServer), GetRoot()); !o.empty() )
			OpcProviderId = GetId( o );
		else{
			let createQL = Ƒ( "createProvider( target:\"{}\", providerType:{} ){{id}}", OpcServer, underlying(EProviderType::OpcServer) );
			OpcProviderId = GetId( QL().QuerySync(createQL, GetRoot()) );
		}
	}

	α login( str loginName, ProviderPK providerId, string opcServer )ε->UserPK{
		using Await = Server::AuthenticateAwait;
		_userId = BlockAwait<Await,UserPK>( Await{loginName, providerId, opcServer} );
		return _userId;
	}

	TEST_F( AuthTests, Login_Existing ){
		const string user{ "Login_Existing" };
		let provider = Access::EProviderType::Google;
		const UserPK userId{ GetId( GetUser(user, GetRoot(), true, (ProviderPK)provider) ) };
		login( user, underlying(provider), {} );
		ASSERT_EQ( userId, _userId );
		PurgeUser( userId, GetRoot() );
	}

	TEST_F( AuthTests, Login_New ){
		const string user{ "Login_New" };
		let userId = login( user, underlying(Access::EProviderType::Google), {} );
		PurgeUser( {userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_Existing_Opc ){
		const string user{ "Login_Existing_Opc" };
		let userId = GetId( GetUser(user, GetRoot(), true, OpcProviderId) );
		login( user, OpcProviderId, string{OpcServer} );
		ASSERT_EQ( UserPK{userId}, _userId );
		PurgeUser( {userId}, GetRoot() );
	}

	TEST_F( AuthTests, Login_New_Opc ){
		const string user{ "Login_New_Opc" };
		let userId = login( user, OpcProviderId, string{OpcServer} );
		PurgeUser( {userId}, GetRoot() );
	}
}