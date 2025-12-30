#pragma once

namespace Jde::Web{ struct Jwt; }
namespace Jde::Opc::Gateway{
	struct UAClient;
namespace Tests{
	struct ITest : ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void;
		Ω TearDownTestCase()ι->void;
		β SetUp()ι->void{}
		constexpr static ELogTags _tags{ ELogTags::Test };

		static optional<Web::Jwt> _jwt;
		static sp<UAClient> _client;
	};
}}