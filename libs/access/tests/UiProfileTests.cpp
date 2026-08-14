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
	class UiProfileTests : public ::testing::Test{};

	Ω upsertProfile( sv target, sv value, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( Ƒ("mutation updateUiProfile( \"target\":\"{}\", \"value\":{} )", target, value), {}, executer );
	}
	Ω deleteProfile( sv target, UserPK executer )ε->jvalue{
		return QL().QuerySync<jvalue>( Ƒ("mutation updateUiProfile( \"target\":\"{}\", \"value\":null )", target), {}, executer );
	}
	Ω selectProfile( sv target, UserPK executer )ε->jobject{
		return QL().QuerySync( Ƒ("uiProfile( target:\"{}\" ){{ value }}", target), {}, executer );
	}

	TEST_F( UiProfileTests, Crud ){
		let root = GetRoot();
		const UserPK userA{ GetId(GetUser("uiProfileA", root)) };
		const UserPK userB{ GetId(GetUser("uiProfileB", root)) };

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

	TEST_F( UiProfileTests, Anonymous ){
		EXPECT_THROW( upsertProfile("anonKey", "\"v\"", UserPK{}), Exception );
		EXPECT_THROW( upsertProfile("anonKey", "\"v\"", UserPK{UserPK::System}), Exception );
	}
}
