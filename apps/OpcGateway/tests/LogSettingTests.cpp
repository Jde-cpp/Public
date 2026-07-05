#include "utils/helpers.h"
#include <thread>

#define let const auto
namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	struct LogSettingTests : public ::testing::Test{
	protected:
		LogSettingTests() {}
		~LogSettingTests() override{}

		Ω SetUpTestCase()ι->void{}
		α SetUp()->void override{}
		α TearDown()->void override{}
	};

	Ω settings()ι->jobject{
		let q = "logSetting{ text binary tags }";
		return Query( q )["data"].as_object();
	}
	Ω defaultLevel(sv logger)ι->ELogLevel{
		let settings = Tests::settings();
		return ToLogLevel( settings.at(logger).at("default").as_string() );
	}
	Ω testUpdate( sv logger )ε->void{
		let settings = Tests::settings();
//		TRACE( "settings: {}", serialize(settings) );
		let currentDefault = ToLogLevel(settings.at(logger).at("default").as_string());
		let newDefault = currentDefault == ELogLevel::Information ? ELogLevel::Warning : ELogLevel::Information;
		jobject vars{ {"default", ToString( newDefault )} };
		auto q = Ƒ("updateLogSetting( {}:{{default: $default}} )", logger);
		auto result = Query( move(q), move(vars) );
		ASSERT_EQ( defaultLevel(logger), newDefault );
	}

	TEST_F( LogSettingTests, UpdateDefault ){
		try{
			//std::this_thread::sleep_for( 600s );
			testUpdate( "text" );
			testUpdate( "binary" );
		//	testUpdate( "appServer" );
			TRACE("~LogSettingTests::UpdateDefault." );
		}
		catch( exception& e ){
			FAIL() << e.what();
		}
	}
}
