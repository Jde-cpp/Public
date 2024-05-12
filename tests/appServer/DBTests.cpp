#include "gtest/gtest.h"
#include "../../../Framework/source/db/Database.h"
#include "../../../Framework/source/um/UM.h"

#define var const auto
namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	class DBTests : public ::testing::Test{
	protected:
		// ThreadingTest() {}
		// ~ThreadingTest() override{}
		Ω SetUpTestCase()->void;
		// void SetUp() override {}
		// void TearDown() override {}
	};
	α DBTests::SetUpTestCase()->void{
		//DB::Execute( "DROP DATABASE IF EXISTS test_jde_app", {} );
		//DB::Execute( "CREATE DATABASE IF NOT EXISTS test_jde_app", {} );
	}
	TEST_F( DBTests, CreateDB ){
		UM::Configure();
	}
}
