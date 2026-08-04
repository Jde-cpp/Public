#include <jde/fwk/settings.h>

#define let const auto

namespace Jde::Tests{
	//Settings::Value() is a jvalue, so every lookup is a json pointer and try_at_pointer rejects one with no leading
	//'/' - a path spelled "cryptoTests/clear" resolves to nothing and the caller's value_or() default wins silently.
	//That is how /http/accessControl/allowOrigin, /server/host, /web/client/timeout and /cryptoTests/clear all sat
	//dead for months.  These pin both halves: the rooted spelling reads the config, the unrooted one reads nothing.
	struct SettingsTests : public ::testing::Test{};

	TEST_F( SettingsTests, RootedPathResolves ){
		EXPECT_EQ( Settings::FindBool("/cryptoTests/clear"), false );//not just has_value - the config sets it to the opposite of every caller's default, so a silent miss is indistinguishable from a hit unless the value is asserted.
		EXPECT_EQ( Settings::FindString("/logging/spd/tags/default"), "Information" );
		EXPECT_EQ( Settings::FindNumber<uint16>("/workers/executor/threads"), 2 );
		EXPECT_TRUE( Settings::FindPath("/testing/file").has_value() );
		EXPECT_NE( Settings::FindObject("/logging/spd"), nullptr );
	}

	//The trap, pinned deliberately: a Find* that comes back empty for a key plainly present in the config means the
	//path is missing its leading '/'.  If tolerance is ever added to Settings, this test is what says so.
	TEST_F( SettingsTests, UnrootedPathResolvesToNothing ){
		EXPECT_FALSE( Settings::FindBool("cryptoTests/clear").has_value() );
		EXPECT_FALSE( Settings::FindString("logging/spd/tags/default").has_value() );
		EXPECT_FALSE( Settings::FindNumber<uint16>("workers/executor/threads").has_value() );
		EXPECT_EQ( Settings::FindObject("logging/spd"), nullptr );
	}

	//a rooted path that matches nothing is still empty - no partial-match fallback.
	TEST_F( SettingsTests, MissingPathIsEmpty ){
		EXPECT_FALSE( Settings::FindBool("/cryptoTests/notAKnob").has_value() );
		EXPECT_FALSE( Settings::FindString("/notASection/notAKnob").has_value() );
	}
}
