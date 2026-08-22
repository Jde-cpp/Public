//ql-review3 #52: AddRemoveAwait::RemoveAfter was a byte-for-byte copy of AddAfter, `co_await Hook::AddAfter` included - so
//IQLHook::RemoveAfter had no caller anywhere and an AddAfter override ran for removes as well as adds.  Latent only because
//nothing in the tree overrides either; QLHookTests.EveryOperationReachesItsOwnVirtual pins the dispatch below this, which was
//always right.  What was missing is a test of the *caller*, which needs a real add and a real remove.
#include "gtest/gtest.h"
#include <jde/ql/QLHook.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	struct CountingMapHook final : QL::IQLHook{
		α AddAfter( const QL::MutationQL&, UserPK, SL )ι->HookResult override{ ++AddAfterCalls; return {}; }
		α RemoveAfter( const QL::MutationQL&, UserPK, SL )ι->HookResult override{ ++RemoveAfterCalls; return {}; }
		uint AddAfterCalls{};
		uint RemoveAfterCalls{};
	};
	//Hook::Add owns it and there is no removal, so it is registered once for the process; counting is harmless to every other
	//test, and each case here resets before it acts.
	Ω mapHook()ι->CountingMapHook&{
		static CountingMapHook& y = []()->CountingMapHook&{
			auto h = mu<CountingMapHook>();
			auto& ref = *h;
			QL::Hook::Add( move(h) );
			return ref;
		}();
		return y;
	}

	//the finding's own reproduction: add then remove, counting both hooks.  It used to read AddAfter=1/Remove=0 then
	//AddAfter=2/Remove=0 - the remove firing the add hook.
	TEST( AddRemoveHookTests, RemoveFiresRemoveAfterAndNotAddAfter ){
		let root = GetRoot();
		let group = GroupPK{ GetId(GetGroup("review52-group", root)) };
		let member = UserPK{ GetId(GetUser("review52-member", root)) };
		mapHook().AddAfterCalls = 0;
		mapHook().RemoveAfterCalls = 0;

		QL().QuerySync<jvalue>( Ƒ("mutation addGroup( id:{}, memberId:{} )", group.Value, member.Value), {}, root );
		EXPECT_EQ( mapHook().AddAfterCalls, 1u );
		EXPECT_EQ( mapHook().RemoveAfterCalls, 0u );

		QL().QuerySync<jvalue>( Ƒ("mutation removeGroup( id:{}, memberId:{} )", group.Value, member.Value), {}, root );
		EXPECT_EQ( mapHook().RemoveAfterCalls, 1u ) << "the remove hook still has no caller";
		EXPECT_EQ( mapHook().AddAfterCalls, 1u ) << "the remove fired the add hook";

		PurgeGroup( group, root );
		PurgeUser( member, root );
	}
}
