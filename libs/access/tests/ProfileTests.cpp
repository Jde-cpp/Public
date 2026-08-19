#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/fwk/str.h>
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class ProfileTests : public ::testing::Test{};

	Ω upsertProfile( sv target, sv value, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( Ƒ("mutation updateProfile( \"target\":\"{}\", \"value\":{} )", target, value), {}, executer );
	}
	Ω deleteProfile( sv target, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( Ƒ("mutation updateProfile( \"target\":\"{}\", \"value\":null )", target), {}, executer );
	}
	Ω selectProfile( sv target, UserPK executer )ε->jobject{
		return QL().QuerySync( Ƒ("profile( target:\"{}\" ){{ value }}", target), {}, executer );
	}

	TEST_F( ProfileTests, Crud ){
		let root = GetRoot();
		const UserPK userA{ GetId(GetUser("profileA", root)) };
		const UserPK userB{ GetId(GetUser("profileB", root)) };

		upsertProfile( "testKey", "\"v1\"", userA );//insert path
		ASSERT_EQ( AsSV(selectProfile("testKey", userA), "value"), "v1" );
		ASSERT_TRUE( selectProfile("testKey", userB).empty() );//scoped to the executer

		upsertProfile( "testKey", "\"{\\\"a\\\":1}\"", userA );//update path - realistic json blob with embedded quotes
		ASSERT_EQ( AsSV(selectProfile("testKey", userA), "value"), "{\"a\":1}" );

		upsertProfile( "testKey", "\"vB\"", userB );//same target, different user - separate rows
		ASSERT_EQ( AsSV(selectProfile("testKey", userB), "value"), "vB" );
		ASSERT_EQ( AsSV(selectProfile("testKey", userA), "value"), "{\"a\":1}" );

		deleteProfile( "testKey", userA );//value:null deletes
		ASSERT_TRUE( selectProfile("testKey", userA).empty() );
		ASSERT_EQ( AsSV(selectProfile("testKey", userB), "value"), "vB" );

		deleteProfile( "testKey", userB );
		PurgeUser( userA, root );
		PurgeUser( userB, root );
	}

	//#10: the save used to be update-then-insert-if-zero.  MySQL's row count is rows *changed*, so re-saving an identical
	//value answered 0, fell into the insert, and the (identity_id,target) pk turned an idempotent save into a 409 - while
	//sqlite and SQL Server, which count rows *matched*, succeeded.  A save is now one dialect upsert, so the second and
	//third writes below are no-ops rather than conflicts on every backend.  This passes on sqlite either way; it is the
	//MySQL path it exists to hold, which no ctest suite reaches.
	TEST_F( ProfileTests, RepeatedIdenticalSave ){
		let root = GetRoot();
		const UserPK user{ GetId(GetUser("profileRepeat", root)) };
		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"same\"", user) );//insert
		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"same\"", user) );//identical - the 409 used to land here
		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"same\"", user) );
		EXPECT_EQ( AsSV(selectProfile("repeatKey", user), "value"), "same" );

		EXPECT_NO_THROW( upsertProfile("repeatKey", "\"changed\"", user) );//a real change still lands
		EXPECT_EQ( AsSV(selectProfile("repeatKey", user), "value"), "changed" );

		deleteProfile( "repeatKey", user );
		PurgeUser( user, root );
	}

	TEST_F( ProfileTests, Anonymous ){
		EXPECT_THROW( upsertProfile("anonKey", "\"v\"", UserPK{}), Exception );
		EXPECT_THROW( upsertProfile("anonKey", "\"v\"", UserPK{UserPK::System}), Exception );
	}
}
