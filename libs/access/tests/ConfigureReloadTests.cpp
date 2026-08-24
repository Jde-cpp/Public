//app-review3 M3: reconnect restored delivery but not state.  Subscriptions::Replay re-issues the event subscriptions, but the
//snapshot loaders ran once at startup and AccessListener only ever applies deltas - so anything that changed while the socket was
//down stayed invisible.  ConfigureAwait's reload mode re-runs the loaders, and the two things that make that safe are what is
//pinned here: it must fill the authorizer from the server, and it must NOT subscribe (Replay already did, and a second
//subscription delivers every event twice).  Run against a throwaway Authorize and listener so the suite's own state is untouched;
//a reload reads only - ResourceSyncAwait, the one step that writes, is skipped.
#include <gtest/gtest.h>
#include <jde/access/AccessListener.h>
#include <jde/access/Authorize.h>
#include <jde/access/awaits/ConfigureAwait.h>
#include <jde/ql/LocalSubscriptions.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{

	Ω reload( sp<Access::Authorize> authorizer, sp<Access::AccessListener> listener )ε->void{
		BlockVoidAwait( ConfigureAwait{QLPtr(), Schemas(), authorizer, UserPK{UserPK::System}, listener, {}, true} );
	}

	TEST( ConfigureReloadTests, ReloadLoadsTheSnapshotWithoutSubscribing ){
		let root = GetRoot();
		let missing = std::to_string( root.Value );//UserName falls back to the pk as a string when Users has no such row.
		auto authorizer = ms<Access::Authorize>( "access" );
		auto listener = ms<Access::AccessListener>( QLPtr() );
		ASSERT_EQ( authorizer->UserName(root), missing ) << "the throwaway authorizer started populated, so this proves nothing";

		reload( authorizer, listener );

		EXPECT_NE( authorizer->UserName(root), missing ) << "the reload did not re-read the identity snapshot";
		//StopListen returns what it removed; nothing to remove means the reload stopped at Acl instead of falling through to Subscribe.
		EXPECT_TRUE( QL::Subscriptions::StopListen(listener).empty() ) << "a reload subscribed a second time - every event would now arrive twice";
	}

	//The half of the reload that is not just "run the loaders again":  Loader::Resources used to `emplace` into maps it never cleared,
	//which keeps the incumbent - so a resource that changed, or was deleted, while the socket was down would have kept its stale entry
	//however many times the loaders ran.
	TEST( ConfigureReloadTests, ReloadReplacesTheResourceMapsRatherThanMergingIntoThem ){
		auto authorizer = ms<Access::Authorize>( "access" );
		auto listener = ms<Access::AccessListener>( QLPtr() );
		reload( authorizer, listener );

		authorizer->AddResource( 0xFFFF, "reload-test", "stale", "" );//stands in for a resource the server no longer returns.
		ASSERT_TRUE( authorizer->FindActiveResourcePK("reload-test", "stale", "") );

		reload( authorizer, listener );
		EXPECT_FALSE( authorizer->FindActiveResourcePK("reload-test", "stale", "") ) << "the reload merged into the resource maps, so a stale entry survived it";
		QL::Subscriptions::StopListen( listener );
	}
}
#undef let
