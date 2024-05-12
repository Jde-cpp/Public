
#define var const auto
namespace Jde::Iot{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	struct DBTests : public ::testing::Test{
	protected:
//		DBTests() {}
//		~DBTests() override{}

//	static α SetUpTestCase()->void;
//	α SetUp()->void override{};
//	α TearDown()->void override{}

		static string DBName;
	};
	string DBTests::DBName{ "tests_jde_iot" };
	
	//α DBTests::SetUpTestCase()->void{	}
	
	TEST_F( DBTests, CreateDB ){
		INFO( "TOIMPL - CreateDB" );
		//DB::CreateDB( DBName );
	}
}