#include <jde/web/client/Jwt.h>
#include <jde/io/Json.h>
#include <jde/Str.h>
#include "../../../../Ssl/source/Ssl.h"

#define var const auto

namespace Jde::Web{
	Jwt::Jwt( Crypto::Modulus mod, Crypto::Exponent exp, string userName, string userTarget, string myEndpoint, string description, sv privateKey )ι:
		Modulus{mod},Exponent{exp},UserName{userName},UserTarget{userTarget},MyEndpoint{myEndpoint},Description{description}{

		array<unsigned char,4> bigEndian;
		for( uint i=0; i<4; ++i )
			bigEndian[i] = (unsigned char)( (exp >> (i*8)) & 0xFF);
		Body = json{
			{ "n", Str::Encode64(mod) },
			{ "e", Str::Encode64(bigEndian) },
			{"name",userName},
			{"target",userTarget},
			{"description",description}
		};
		var head = json{ {"alg","RS256"}, {"typ","JWT"} };
		HeaderBodyEncoded = Str::Encode64( head.dump() + "." + Body.dump() );
		Fingerprint = Crypto::RsaSign( HeaderBodyEncoded, privateKey );
	}
	Jwt::Jwt( str encoded )ε{
		var fpIndex = encoded.find_last_of( '.' );
		var bodyIndex = encoded.find_first_of( '.' );
		if( fpIndex==string::npos || fpIndex==encoded.size() || bodyIndex==string::npos )
			THROW( "Invalid jwt.  Expected 3 parts." );
		HeaderBodyEncoded = encoded.substr( 0, fpIndex );
		var header = Json::Parse( Ssl::Decode64(HeaderBodyEncoded.substr(0, bodyIndex), true) );//{"alg":"RS256","kid":"fed80fec56db99233d4b4f60fbafdbaeb9186c73","typ":"JWT"}
		if( auto alg = Json::Getε(header, "alg"); alg!="RS256" )
			THROW( "Invalid jwt.  Expected alg=RS256, found '{}'.", alg );
		if( auto type = Json::Getε(header, "typ"); type!="JWT" )
			THROW( "Invalid jwt.  Expected typ=JWT, found '{}'.", type );
		Kid = Json::Get( header, "kid" );
		Fingerprint = Ssl::Decode64<Crypto::Signature>( encoded.substr(fpIndex+1), true );
		Body = Json::Parse( Ssl::Decode64(HeaderBodyEncoded.substr(bodyIndex+1), true) );
		SetModulus( Json::Getε(Body, "n") );
		SetExponent( Json::Getε(Body, "e") );
		auto fpKey = Crypto::Fingerprint( Modulus, Exponent );
		UserName = Json::TryGet(Body, "name").value_or( Str::ToHex(fpKey.data(), fpKey.size()) );
		UserTarget = Json::TryGet( Body, "target" ).value_or( UserName );
		Host = Json::Get( Body, "host" );
		Iat = Json::Get<time_t>( Body, "iat" );

		auto fpText = []( const Crypto::MD5& fp ) {
			return std::accumulate(fp.begin(), fp.end(), 𐢜("{:x}", fp[0]), [&](string s, byte b){return move(s)+𐢜("{:x}", fp[0]);} );
		};
		Description = Json::TryGet(Body, "description").value_or( 𐢜("Public key md5: {}", fpText(fpKey)) );
	}

	α Jwt::SetModulus( str encoded )ι->void{
		Modulus = Ssl::Decode64<vector<unsigned char>>( encoded, true );
		// vector<char> temp{ Ssl::Decode64<vector<char>>(encoded, true) };
		// std::copy( (byte*)temp.data(), (byte*)temp.data()+temp.size(), Modulus.begin() );
	}
	α Jwt::SetExponent( str encoded )ι->void{
		var decoded = Ssl::Decode64<string>( encoded, true );
		for( uint i=0; i<std::min(4ul,decoded.size()); ++i )
			Exponent |= decoded[i] << (i*8);
	}
}