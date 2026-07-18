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
