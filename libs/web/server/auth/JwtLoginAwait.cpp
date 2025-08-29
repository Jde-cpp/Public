#include <jde/web/server/auth/JwtLoginAwait.h>
#include <jde/access/server/awaits/LoginAwait.h>
#include "GoogleLoginAwait.h"

#define let const auto
namespace Jde::Web::Server{
	α JwtLoginAwait::Execute()ι->TAwait<UserPK>::Task{
		try{
			//THROW_IF( std::abs(time(nullptr)-_jwt.Iat)>60*10, "Invalid iat.  Expected ~'{}', found '{}'.", time(nullptr), _jwt.Iat );
			UserPK userPK{};
			if( _jwt.Iss()=="https://accounts.google.com" ){
				THROW_IF( !_isAppServer, "Google login only implemented on Application Server." );
				userPK = co_await GoogleLoginAwait( move(_jwt) );
			}
			else{
				Crypto::Verify( _jwt.PublicKey, _jwt.HeaderBodyEncoded, _jwt.Signature );
				userPK = co_await Access::Server::LoginAwait( move(_jwt.PublicKey), move(_jwt.UserName), move(_jwt.UserTarget), move(_jwt.Description), {} );
			}
			Resume( move(userPK) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}