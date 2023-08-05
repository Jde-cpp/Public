#include <codecvt>
#include "gtest/gtest.h"

namespace Jde::ML
{
#define var const auto
	using nlohmann::json;
	struct MainTests : public ::testing::Test
	{
	protected:
		MainTests()noexcept{}
		~MainTests()noexcept override{}

		void SetUp()noexcept override { /*DB::Create Schema();*/}
		void TearDown()noexcept override {}
	};

	TEST_F( MainTests, Process )
	{
		try
		{
		}
		catch( const IException& e )
		{
			e.Log();
		}
	}
}