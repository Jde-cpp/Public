//opcserver-review3 #16:  an app client's acl, listener and configure context were process-wide statics, so every IAppClient
//in one process (Jde.Opc.Tests hosts three) shared the first caller's acl and the last caller's context.  Members now.
#include <gtest/gtest.h>
#include <jde/access/Authorize.h>
#include <jde/app/client/IAppClient.h>

#define let const auto

namespace Jde::App::Tests{
	struct BareAppClient final : Client::IAppClient{
		α ClientQuery( QL::RequestQL&&, Jde::UserPK, SL )ε->up<TAwait<jvalue>> override{ return nullptr; }
	};
	struct SubclassAuthorize final : Access::Authorize{ using Access::Authorize::Authorize; };//stands in for the OpcServer's OpcAuthorize.

	TEST( IAppClientTests, AclIsPerClient ){
		auto a = ms<BareAppClient>(); auto b = ms<BareAppClient>();
		ASSERT_FALSE( a->Acl() );//null until asked for - clients that never authorize (emulator, soak) keep none.
		auto acl = a->Acl( "a" );
		ASSERT_TRUE( acl );
		ASSERT_EQ( acl, a->Acl() );
		ASSERT_EQ( acl, a->Acl("ignored") );//the first name sticks.
		ASSERT_FALSE( b->Acl() );//a's acl is not b's.
		ASSERT_NE( acl, b->Acl("b") );
	}
	TEST( IAppClientTests, SetAclReplacesOnlyItsOwn ){
		auto a = ms<BareAppClient>(); auto b = ms<BareAppClient>();
		auto base = b->Acl( "b" );
		auto sub = ms<SubclassAuthorize>( "a" );
		a->SetAcl( sub );
		ASSERT_EQ( a->Acl(), sub );
		ASSERT_EQ( b->Acl(), base );//the OpcServer's SetAcl(OpcAuthorize) no longer hands its subclass to the gateway.
	}
	TEST( IAppClientTests, ReloadBeforeConfigureThrows ){
		auto a = ms<BareAppClient>();
		ASSERT_FALSE( a->IsAccessConfigured() );//was a static optional:  configured by any client meant configured for all.
		ASSERT_THROW( a->ReloadAccess(), std::exception );
	}
}
