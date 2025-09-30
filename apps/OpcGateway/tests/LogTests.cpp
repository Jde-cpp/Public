#include <jde/fwk/log/MemoryLog.h>
#include <jde/app/log/ProtoLog.h>
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	struct LogTests : public ::testing::Test{
	protected:
		LogTests() {}
		~LogTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{}
		α TearDown()->void override {}
	};

	TEST_F( LogTests, Exists ){
		auto pp = find_if( Logging::Loggers(), []( auto& l ){ return dynamic_cast<App::ProtoLog*>(l.get())!=nullptr; } );
		ASSERT_TRUE( pp!=Logging::Loggers().end() );
		//auto& protoLogger = (App::ProtoLog&)**pp;
		//ASSERT_TRUE( protoLogger.Load().size() );
	}
}
