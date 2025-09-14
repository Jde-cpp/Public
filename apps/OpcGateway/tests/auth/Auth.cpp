#include "Auth.h"
#include "../../src/auth/UM.h"
#include "../../src/usings.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	optional<ServerCnnctn> Auth::Connection={};
	ETokenType Auth::Tokens{};
	α Auth::SetUp()ε->void{
		if( !Connection )
			Connection = GetConnection( OpcServerTarget );
		if( empty(Tokens) )
			Tokens = AvailableUserTokens( Connection->Url );
		if( empty(Tokens & ETokenType(TokenType)) ){
			GTEST_SKIP() << "Authentication type is not allowed on this server.";
		}
	}
	α Auth::TearDownTestSuite()->void{
		if( auto client = SelectServerCnnctn(OpcServerTarget); client ){
			PurgeServerCnnctn( client->Id );
			Connection = nullopt;
		}
	}
}