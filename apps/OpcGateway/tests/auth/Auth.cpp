#include "Auth.h"
#include "../../src/auth/UM.h"
#include "../../src/usings.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	optional<ServerCnnctn> Auth::Client={};
	ETokenType Auth::Tokens{};
	α Auth::SetUp()ε->void{
		if( !Client ){
			Client = SelectServerCnnctn( OpcServerTarget );
			if( !Client ){
				BlockAwait<ProviderCreatePurgeAwait, Access::ProviderPK>( ProviderCreatePurgeAwait{OpcServerTarget, false} );
				let id = BlockAwait<CreateServerCnnctnAwait, ServerCnnctnPK>( CreateServerCnnctnAwait{} );
				Client = SelectServerCnnctn( id );
			}
		}
		if( empty(Tokens) )
			Tokens = AvailableUserTokens( Client->Url );
		if( empty(Tokens & ETokenType(TokenType)) ){
			GTEST_SKIP() << "Authentication type is not allowed on this server.";
		}
	}
	α Auth::TearDownTestSuite()->void{
		if( auto client = SelectServerCnnctn(OpcServerTarget); client ){
			PurgeServerCnnctn( client->Id );
			Client = nullopt;
		}
	}
}