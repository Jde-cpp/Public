
#define var const auto
namespace Jde::Iot{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	struct OpcUserTests : public ::testing::Test{
	protected:

		static string User;
	};
	string OpcUserTests::User{ "joe" };
	
	//α DBTests::SetUpTestCase()->void{	}
	
	TEST_F( OpcUserTests, CreateOpcUser ){
		INFO( "TOIMPL - CreateUser" );
		//DB::CreateDB( DBName );
	}
	TEST_F( OpcUserTests, CreateUser ){
		INFO( "TOIMPL - CreateUser" );
		//DB::CreateDB( DBName );
	}

}