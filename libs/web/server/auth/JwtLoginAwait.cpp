#include <jde/web/server/auth/JwtLoginAwait.h>
#include <jde/access/server/awaits/LoginAwait.h>
#include "GoogleLoginAwait.h"

#define let const auto
namespace Jde::Web::Server{
	α JwtLoginAwait::Execute()ι->TAwait<UserPK>::Task{
		try{
			//THROW_IF( std::abs(time(nullptr)-_jwt.Iat)>60*10, "Invalid iat.  Expected ~'{}', found '{}'.", time(nullptr), _jwt.Iat );
			UserPK userPK{};
			if( _appClient->IsLocal() ){
				if( _jwt.Iss()=="https://accounts.google.com" )
					userPK = co_await GoogleLoginAwait( move(_jwt) );
				else{
					Crypto::Verify( _jwt.PublicKey, _jwt.HeaderBodyEncoded, _jwt.Signature );
					userPK = co_await Access::Server::LoginAwait( move(_jwt.PublicKey), move(_jwt.UserName), move(_jwt.UserTarget), move(_jwt.Description), {} );
				}
				ResumeScaler( userPK );
			}
			else
				LoginAppServer();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α JwtLoginAwait::LoginAppServer()ι->Client::ClientSocketAwait<FromServer::SessionInfo>::Task{
		try{
			auto session = co_await _appClient->Login( move(_jwt), _sl );
			ResumeScaler( {session.user_pk()} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}