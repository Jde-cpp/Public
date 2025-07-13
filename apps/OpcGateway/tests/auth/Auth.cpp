#include "Auth.h"
#include "../../src/auth/UM.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	α Auth::SetUp()ε->void{
		if( OpcServer.empty() ){
			OpcServer = SelectOpcClient( OpcServerTarget );
			if( OpcServer.empty() ){
				BlockAwait<ProviderCreatePurgeAwait, Access::ProviderPK>( ProviderCreatePurgeAwait{OpcServerTarget, false} );
				let id = BlockAwait<CreateOpcClientAwait, OpcClientPK>( CreateOpcClientAwait{} );
				Auth::OpcServer = SelectOpcClient( id );
			}
			Auth::_authAllowed = HasUserToken( Auth::OpcServer.at("url").as_string(), _tokenType );
		}
		if( !Auth::_authAllowed ){
			GTEST_SKIP() << "Authentication type is not allowed on this server.";
		}
	}
}