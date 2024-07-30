#pragma once
#include <jde/crypto/OpenSsl.h>

namespace Jde::Web{
	struct Jwt{
		Jwt( str jwt )ε;
		Jwt( Crypto::Modulus mod, Crypto::Exponent exp, str userName, str userTarget, str myEndpoint, str description, const fs::path& privateKeyPath )ι;
		α Payload()Ι->string;
		string Kid;
		json Body;
		string HeaderBodyEncoded;
		Crypto::Signature Signature;
		Crypto::Modulus Modulus;
		Crypto::Exponent Exponent;

		string Host;
		time_t Iat;
		string UserName;
		string UserTarget;
		string Description;
	private:
		α SetModulus( str encoded )ι->void;
		α SetExponent( str encoded )ι->void;
	};
}