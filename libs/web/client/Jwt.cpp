#include <jde/web/Jwt.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/io/json.h>
#include <jde/fwk/str.h>

#define let const auto

namespace Jde::Web{
	Jwt::Jwt( Crypto::PublicKey key, Jde::UserPK userPK, str userName, str userTarget, SessionPK sessionId, str endpoint, TimePoint expires, str description, const fs::path& privateKeyPath )ι:
		PublicKey{ move(key) }, Host{ endpoint }, Iat{ time(nullptr) }, UserPK{ userPK }, UserName{ userName }, UserTarget{ userTarget }, Description{ description }{
		Body = jobject{
			{ "n", Str::Encode64(PublicKey.Modulus, true) },
			{ "e", Str::Encode64(PublicKey.Exponent, true) },
			{ "iat", Iat },
			{ "host", Host },
			{ "sub", UserPK.Value },
			{ "name", userName },
			{ "target", userTarget },
		};
		if( sessionId )
			Body["sid"] = hex( sessionId );
		if( description.size() )
			Body["description"] = Description;
		if( expires!=TimePoint::min() )
			Body["exp"] = Clock::to_time_t( expires );

		let head = jobject{ {"alg","RS256"}, {"typ","JWT"} };
		HeaderBodyEncoded = Str::Encode64( serialize(head), true )+ "." + Str::Encode64( serialize(Body), true );
		Signature = Crypto::RsaSign( HeaderBodyEncoded, privateKeyPath );
	}
	Jwt::Jwt( sv encoded )ε{
		let fpIndex = encoded.find_last_of( '.' );
		let bodyIndex = encoded.find_first_of( '.' );
		if( fpIndex==string::npos || fpIndex==encoded.size() || bodyIndex==string::npos || fpIndex==bodyIndex )
			THROW( "Invalid jwt.  Expected 3 parts." );
		HeaderBodyEncoded = encoded.substr( 0, fpIndex );
		let headerEncoded = HeaderBodyEncoded.substr( 0, bodyIndex );
		let header = Json::Parse( Str::Decode64(headerEncoded, true) );//{ "alg":"RS256","kid":"fed80fec56db99233d4b4f60fbafdbaeb9186c73","typ":"JWT" }
		if( auto alg = Json::AsSV(header, "alg"); alg!="RS256" )
			THROW( "Invalid jwt.  Expected alg=RS256, found '{}'.", alg );
		if( auto type = Json::AsSV(header, "typ"); type!="JWT" )
			THROW( "Invalid jwt.  Expected typ=JWT, found '{}'.", type );
		Kid = Json::FindSV( header, "kid" ).value_or( "" );
		let fp = encoded.substr( fpIndex+1 );
		Signature = Str::Decode64<Crypto::Signature>( fp.substr(0, fp.find_first_of('=')), true );

		auto body = Str::Decode64( HeaderBodyEncoded.substr(bodyIndex+1), true );
		Body = Json::Parse( body );
		optional<Crypto::MD5> fpKey;
		if( auto modulus = Json::FindString(Body, "n"); modulus ){
			SetModulus( move(*modulus) );
			SetExponent( Json::AsString(Body, "e") );
			fpKey = Crypto::Fingerprint( PublicKey );// Use PublicKey instead of Certificate
		}
		UserPK = { Json::FindNumber<UserPK::Type>(Body, "sub").value_or(0) };
		UserName = Json::FindString( Body, "name" ).value_or( fpKey ? Str::ToHex((byte*)fpKey->data(), fpKey->size()) : "" );
		UserTarget = Json::FindString( Body, "target" ).value_or( UserName );
		Host = Json::FindString( Body, "host" ).value_or( "" );
		Iat = Json::AsNumber<time_t>( Body, "iat" );
		SessionId = Json::FindDefaultSV( Body, "sid" );

		Description = Json::FindSV( Body, "description" ).value_or( fpKey ? Ƒ("Public key md5: {}", boost::uuids::to_string(*fpKey)) : "" );
	}
	α Jwt::Payload()Ι->string{
		auto signature = Str::Encode64( Signature, true );
		auto payload = Ƒ( "{}.{}", HeaderBodyEncoded, signature );
		return payload;
	}
	α Jwt::SetModulus( str encoded )ι->void{
		PublicKey.Modulus = Str::Decode64<Crypto::Modulus>( encoded, true );
	}
	α Jwt::SetExponent( str encoded )ι->void{
		PublicKey.Exponent = Str::Decode64<Crypto::Exponent>( encoded, true );
	}
}