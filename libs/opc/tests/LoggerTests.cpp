//EOpcLogTags extends fwk's ELogTags into bits 32-45.  UALogParser is what makes those bits addressable by name from a
//jsonnet config or a remote log subscription, and main.cpp registers it before Process::Startup.
#include <gtest/gtest.h>
#include <jde/opc/uatypes/Logger.h>

#define let const auto

namespace Jde::Opc::Tests{
	TEST( UALogParserTests, TagNamesRoundTrip ){
		UALogParser parser;
		EXPECT_EQ( parser.ToTag("opc"), (ELogTags)EOpcLogTags::Opc );
		EXPECT_EQ( parser.ToTag("uaClient"), (ELogTags)EOpcLogTags::Client );
		EXPECT_EQ( parser.ToTag("monitoring"), (ELogTags)EOpcLogTags::Monitoring );
		EXPECT_EQ( parser.ToTag("browse"), (ELogTags)EOpcLogTags::Browse );
		EXPECT_EQ( parser.ToTag("processingLoop"), (ELogTags)EOpcLogTags::ProcessingLoop );
		EXPECT_EQ( parser.ToTag("nope"), ELogTags{} ); //an unknown name yields no bits, it does not guess.

		EXPECT_EQ( parser.ToString((ELogTags)EOpcLogTags::Opc), "opc" );
		EXPECT_EQ( parser.ToString((ELogTags)EOpcLogTags::Browse), "browse" );
		EXPECT_EQ( parser.ToString(ELogTags::App), "" );  //a plain fwk tag is not ours to name.
		EXPECT_EQ( parser.Tags().size(), 14u );
	}

	TEST( UALogParserTests, CombinedTagsSerializeInBitOrder ){
		UALogParser parser;
		let both = (ELogTags)( (uint)EOpcLogTags::Opc | (uint)EOpcLogTags::Browse );
		EXPECT_EQ( parser.ToString(both), "opc.browse" );
	}

	//The framework consults the registered parsers, so the jsonnet `logging.spd.tags` block can name opc tags.
	TEST( UALogParserTests, RegisteredWithTheFramework ){
		EXPECT_EQ( ToLogTags(sv{"browse"}), (ELogTags)EOpcLogTags::Browse );
		EXPECT_EQ( ToLogTags(sv{"opc"}), (ELogTags)EOpcLogTags::Opc );
		EXPECT_TRUE( Logging::Tags().contains("monitoring") );
		EXPECT_TRUE( Logging::Tags().contains("app") ); //fwk's own tags are still there.
	}

	//The composite tags the library hands to the log macros.
	TEST( UALogParserTests, CompositeTags ){
		EXPECT_EQ( (uint)IotReadTag & (uint)ELogTags::Read, (uint)ELogTags::Read );
		EXPECT_EQ( (uint)IotReadTag & (uint)EOpcLogTags::Opc, (uint)EOpcLogTags::Opc );
		EXPECT_EQ( (uint)DataChangesTag & (uint)ELogTags::Pedantic, (uint)ELogTags::Pedantic );
		EXPECT_EQ( (uint)DataChangesTag & (uint)EOpcLogTags::Monitoring, (uint)EOpcLogTags::Monitoring );
		EXPECT_EQ( (uint)BrowseTagPedantic & (uint)EOpcLogTags::Browse, (uint)EOpcLogTags::Browse );
	}

	//Every EOpcLogTags bit is above 31 - they extend the fwk space rather than colliding with it.
	TEST( UALogParserTests, TagsExtendRatherThanOverlapTheFrameworkSpace ){
		for( let& [name, bit] : UALogParser{}.Tags() )
			EXPECT_EQ( bit & 0xFFFFFFFFull, 0u ) << name;
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#14: fwk spells ELogTags::All as ~0ul.  `unsigned long` is 32-bit under the windows ABI while the enum's underlying
	//type is 64-bit, so All is 0xFFFFFFFF and masks off every opc tag - SubscribeLog's default Tags{ELogTags::All} drops
	//the whole opc/uaClient/monitoring/browse channel on the win-clang build only (this passes as written on linux).
	//Fix: `All = ~0ull` in fwk's logTags.h.
	TEST( UALogParserTests, DISABLED_R2_14_AllIncludesTheOpcTags ){
		EXPECT_NE( (uint)ELogTags::All & (uint)EOpcLogTags::Opc, 0u );
		EXPECT_NE( (uint)ELogTags::All & (uint)EOpcLogTags::Browse, 0u );
		EXPECT_NE( (uint)ELogTags::All & (uint)EOpcLogTags::Monitoring, 0u );
	}
}
