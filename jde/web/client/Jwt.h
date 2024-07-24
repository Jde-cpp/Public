#pragma once
#include <jde/crypto/OpenSsl.h>

namespace Jde::Web{
	struct Jwt{
		Jwt( str jwt )ε;
		Jwt( Crypto::Modulus mod, Crypto::Exponent exp, string userName, string userTarget, string myEndpoint, string description, sv privateKey )ι;
		string Kid;
		json Body;
		string HeaderBodyEncoded;
		Crypto::Signature Fingerprint;
		Crypto::Modulus Modulus;
		Crypto::Exponent Exponent;

		string Host;
		time_t Iat;
		string UserName;
		string UserTarget;
		string MyEndpoint;
		string Description;
	private:
		α SetModulus( str encoded )ι->void;
		α SetExponent( str encoded )ι->void;
	};
}