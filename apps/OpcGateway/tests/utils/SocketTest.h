#pragma once

namespace Tests{
	struct ITest : ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void;
		Ω TearDownTestCase()ι->void;
		β SetUp()ι->void{}
		constexpr static ELogTags _tags{ ELogTags::Test };
	};
}