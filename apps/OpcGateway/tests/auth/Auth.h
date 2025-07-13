#include "../helpers.h"

namespace Jde::Opc::Gateway::Tests{
	class Auth : public ::testing::Test{
	protected:
		Auth( UA_UserTokenType tt )ι:_tokenType{tt}{}
		~Auth()override{}

		//Ω SetUpTestCase()ε->void;
		//Ω CheckPasswordsAllowed()ε->void;
		α SetUp()ε->void override;
		α TearDown()ι->void override{}
		//Ω TearDownTestSuite();
	public:
		jobject OpcServer;
		bool _authAllowed;
		UA_UserTokenType _tokenType;
	};
}