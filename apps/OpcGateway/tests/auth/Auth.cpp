#include "Auth.h"
#include "../../src/auth/UM.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	optional<OpcClient> Auth::Client={};
	ETokenType Auth::Tokens{};
	α Auth::SetUp()ε->void{
		if( !Client ){
			Client = SelectOpcClient( OpcServerTarget );
			if( !Client ){
				BlockAwait<ProviderCreatePurgeAwait, Access::ProviderPK>( ProviderCreatePurgeAwait{OpcServerTarget, false} );
				let id = BlockAwait<CreateOpcClientAwait, OpcClientPK>( CreateOpcClientAwait{} );
				Client = SelectOpcClient( id );
			}
		}
		if( empty(Tokens) )
			Tokens = AvailableUserTokens( Client->Url );
		if( empty(Tokens & ETokenType(TokenType)) ){
			GTEST_SKIP() << "Authentication type is not allowed on this server.";
		}
	}
	α Auth::TearDownTestSuite()->void{
		if( auto client = SelectOpcClient(OpcServerTarget); client ){
			PurgeOpcClient( client->Id );
			Client = nullopt;
		}
	}
}