#include "../helpers.h"

namespace Jde::Opc::Gateway::Tests{
	class Auth : public ::testing::Test{
	protected:
		Auth( ETokenType tt )ι:TokenType{tt}{}
		~Auth()override{}

		α SetUp()ε->void override;
		α TearDown()ι->void override{}
		Ω TearDownTestSuite()->void;
	public:
		static optional<ServerCnnctn> Connection;
		static ETokenType Tokens;
		ETokenType TokenType;
	};
}