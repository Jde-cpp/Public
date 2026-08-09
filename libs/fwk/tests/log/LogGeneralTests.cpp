#include "jde/fwk/log/ILogger.h"
#include <jde/fwk/log/MemoryLog.h>
#define let const auto
#pragma warning( disable: 4702 )

namespace Jde::Tests{
	struct LogGeneralTests : public ::testing::Test{
	protected:
		LogGeneralTests() {}
		~LogGeneralTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{ Logging::ClearMemory(); }
		α TearDown()->void override {}
	};

	//ELogTags is a 64-bit space that libraries extend above fwk's own 27 bits - Jde::Opc registers 32-45 through
	//AddTagParser - so All has to span the whole underlying type.  Spelled ~0ul it stopped at bit 31 under the windows
	//ABI, where unsigned long is 32-bit, and silently dropped every extension tag from an All-based subscription.
	TEST_F( LogGeneralTests, AllCoversTheFullUnderlyingWidth ){
		EXPECT_EQ( (uint)ELogTags::All, ~(std::underlying_type_t<ELogTags>)0 );
		EXPECT_NE( (uint)ELogTags::All & (1ull<<32), 0ull );//the bit range that actually went missing.
	}

	//AddTagParser takes ownership as up<ITagParser>, so every parser is destroyed through a base pointer.  Without a
	//virtual destructor on the base that is undefined behaviour, and in practice the derived destructor never runs - it
	//goes unnoticed only because the one implementation, Jde::Opc::UALogParser, holds no state to leak.
	TEST_F( LogGeneralTests, TagParserIsDestroyedThroughTheBase ){
		static bool destroyed;//a local class may use a static local of its enclosing function, not an automatic one.
		struct Counted : Logging::ITagParser{
			~Counted(){ destroyed = true; }
			α ToTag( str )Ι->ELogTags override{ return ELogTags::None; }
			α ToString( ELogTags )Ι->string override{ return {}; }
			α Tags()Ι->flat_map<string,uint> override{ return {}; }
		};
		destroyed = false;
		up<Logging::ITagParser> parser = mu<Counted>();//the conversion AddTagParser performs.
		parser.reset();
		EXPECT_TRUE( destroyed );
	}

	//config keys are split on '_' and each part looked up in ELogTagStrings, so an unrecognised part is simply dropped
	//and the key silently collapses onto a shorter one's mask - where parseTags' insert_or_assign then overwrites it.
	TEST_F( LogGeneralTests, CompositeTagNamesResolveDistinctly ){
		let tags = []( sv name ){ return ToLogTags( name ); };//sv, not a literal - ToLogTags(sv)/ToLogTags(jvalue) are ambiguous for const char*.
		EXPECT_EQ( tags("socket_client_read_subscription"), ELogTags::SocketClientReadSub );
		EXPECT_EQ( tags("socket_client_write_subscription"), ELogTags::SocketClientWriteSub );
		//the collision the "_sub" spelling caused: it must not resolve to the plain read/write tag.
		EXPECT_NE( tags("socket_client_read_subscription"), tags("socket_client_read") );
		EXPECT_NE( tags("socket_client_write_subscription"), tags("socket_client_write") );
		EXPECT_EQ( tags("socket_client_read"), ELogTags::SocketClientRead );
		//an unknown part contributes nothing, which is exactly how "_sub" aliased onto socket_client_read.
		EXPECT_EQ( tags("socket_client_read_sub"), ELogTags::SocketClientRead );
	}

	TEST_F( LogGeneralTests, CachedTags ){
		auto& logger = Logging::GetLogger<Logging::MemoryLog>();

		let _tags = ELogTags::Scheduler;
		Logging::ClearMemory();
		constexpr auto logMessage = "scheduler msg";
		TRACE( logMessage );
		ASSERT_TRUE( logger.Find(logMessage).empty() );
		logger.SetLevel( _tags, ELogLevel::Debug );
		DBG( logMessage );
		ASSERT_FALSE( logger.Find(logMessage).empty() );
	}

	TEST_F( LogGeneralTests, ArgsNotCalled ){
		auto& logger = Logging::GetLogger<Logging::MemoryLog>();
		let unConfiguredTags = ELogTags::Scheduler;
		logger.SetLevel( unConfiguredTags, ELogLevel::Error );
		auto arg = []()->string {
			throw std::runtime_error("should not be called");
			return "";
		};
		ASSERT_NO_THROW( TRACET(unConfiguredTags, "{}", arg()) );
	}

	//TODO makesure cumulative is updated when some obscure tag is sent.
	//How quick is md5 calculation.
	//How quick is noop.
	//How quick is memory.
	//How quick is spdlog.
}
