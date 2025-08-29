#include "GoogleLoginAwait.h"
#include <jde/crypto/OpenSsl.h>
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
#include <jde/access/server/accessServer.h>
#include <jde/access/types/GoogleTokenInfo.h>

#define let const auto
namespace Jde::Web::Server{
	using Web::Client::ClientHttpAwait;
	up<jobject> jwks;

	α GoogleLoginAwait::Execute()ι->ClientHttpAwait::Task{
		try{
			if( !jwks ){
				jwks = mu<jobject>( (co_await ClientHttpAwait{
					"www.googleapis.com",
					string{"/oauth2/v3/certs"},
					443,
					{.ContentType="", .Verb=http::verb::get}} ).Json() );
			}
			THROW_IF( _jwt.Aud() != Settings::FindSV("/http/clientSettings/googleAuthClientId").value_or(""), "Invalid client id: '{}'", _jwt.Aud() );

			let& kid = _jwt.Kid;
			THROW_IF( kid.empty(), "Could not find kid in header {}", Str::Decode64<string>(_jwt.HeaderBodyEncoded) );
			let& keys = Json::AsArray( *jwks, "keys" );
			jobject foundKey;
			for( let& key : keys ){
				let keyString = Json::AsString( Json::AsObject(key), "kid" );
				if( keyString==kid )
					foundKey = Json::AsObject(key);
			}
			THROW_IF( foundKey.empty(), "Could not find key... '{}' in: '{}'", kid, serialize(keys) );
			_jwt.SetModulus( Json::AsString( foundKey, "n") );
			_jwt.SetExponent( Json::AsString( foundKey, "e") );
			Crypto::Verify( _jwt.PublicKey, _jwt.HeaderBodyEncoded, _jwt.Signature );
			Authenticate();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α GoogleLoginAwait::Authenticate()->TAwait<UserPK>::Task{
		Google::TokenInfo token{ _jwt.Body };
#ifdef NDEBUG
		//let expiration = Clock::from_time_t(token.Expiration);
		//THROW_IF(expiration < Clock::now(), "token expired");
#endif
		ResumeScaler( co_await Access::Server::Authenticate(token.Email, underlying(Access::EProviderType::Google)) );
	}
}