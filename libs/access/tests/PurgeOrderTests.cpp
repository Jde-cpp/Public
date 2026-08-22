//ql-review3 #25: PurgeAwait's only Authorize was inside Statements(), which runs *after* Before() has already co_awaited
//Hook::PurgeBefore - so a purge hook did its work (on the gateway: removing the access provider, then re-creating it from
//PurgeFailure under a new id) and only then was the caller refused.  Every other op authorizes before its hook.
//The hook here only counts, so leaving it registered for the rest of the suite costs nothing.
#include "gtest/gtest.h"
#include <jde/ql/QLHook.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	static std::atomic<uint> _purgeBeforeCount{};
	struct CountingPurgeHook final : QL::IQLHook{
		α PurgeBefore( const QL::MutationQL& m, UserPK, SL )ι->HookResult override{
			if( m.TableName()=="groups" )
				++_purgeBeforeCount;
			return {};
		}
	};

	struct PurgeOrderTests : ::testing::Test{
		Ω SetUpTestCase()->void{ QL::Hook::Add( mu<CountingPurgeHook>() ); }
		α SetUp()->void override{
			_intruder = UserPK{ GetId(Tests::Get("user", "review25-intruder", GetRoot())) };
			let resource = SelectResource( "groups", GetRoot(), true );
			_wasDeleted = !resource.at( "deleted" ).is_null();
			if( _wasDeleted )
				Restore( "resources", GetId(resource), GetRoot() );//a resource is enforced only while it is not deleted.
			_group = GroupPK{ GetId(GetGroup("review25-group", GetRoot())) };
		}
		α TearDown()->void override{
			if( !Select("group", _group.Value, GetRoot(), "id", true).empty() )
				PurgeGroup( _group, GetRoot() );
			if( _wasDeleted )
				Delete( "resources", GetId(SelectResource("groups", GetRoot(), true)), GetRoot() );
			PurgeUser( _intruder, GetRoot() );
		}
		UserPK _intruder;
		GroupPK _group{};
		bool _wasDeleted{};
	};

	TEST_F( PurgeOrderTests, UnauthorizedPurgeNeverReachesTheHook ){
		let before = _purgeBeforeCount.load();
		EXPECT_THROW( QL().QuerySync<jvalue>("mutation purgeGroup( id:"+std::to_string(_group.Value)+" )", {}, _intruder), Exception );
		EXPECT_EQ( _purgeBeforeCount.load(), before ) << "the purge hook ran before the caller was refused";
		EXPECT_FALSE( Select("group", _group.Value, GetRoot(), "id", true).empty() ) << "the group was purged anyway";

		//the control: an authorized purge still runs the hook, so the check above is ordering and not a hook that never fires.
		QL().QuerySync<jvalue>( "mutation purgeGroup( id:"+std::to_string(_group.Value)+" )", {}, GetRoot() );
		EXPECT_EQ( _purgeBeforeCount.load(), before+1 );
	}
}
