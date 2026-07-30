#pragma once
//#include "OpenSsl.h"
#include <jde/fwk/chrono.h>
#define Φ Γ auto

namespace Jde::Crypto{
	using Modulus = vector<unsigned char>;
	using Exponent = vector<unsigned char>;

	struct PrivateKeySettings{
		PrivateKeySettings( fs::path path, string passcode )ι: Path{move(path)}, Passcode{move(passcode)}{}
		PrivateKeySettings( const jobject& settings, optional<sv> defaultFileName )ι;
		fs::path Path;
		string Passcode;
	};

	struct PublicKey{
		PublicKey() = default;
		PublicKey( fs::path path, SRCE )ε;
		PublicKey( Crypto::Modulus modulus, Crypto::Exponent exponent )ε: Modulus{move(modulus)}, Exponent{move(exponent)} {}
		α operator==( const PublicKey& other )Ι->bool{ return Modulus == other.Modulus && Exponent == other.Exponent; }
		α operator<( const PublicKey& other )Ι->bool{ return Exponent == other.Exponent ? Modulus < other.Modulus : Exponent < other.Exponent; }
		Φ Hash32()Ι->uint32_t;
		Φ ExponentInt()Ι->uint32_t;
		Φ ModulusHex()Ε->string;
		α ToBytes()ε->vector<byte>;
		Crypto::Modulus Modulus;
		Crypto::Exponent Exponent;
	};

	struct Γ Certificate{
		Certificate( const jobject& settings, optional<sv> defaultFileName )ι;
		Certificate( std::span<const byte> certificate, SRCE )ε;
		α Log( string prefix, SRCE )Ι->void;
		α ToString()Ι->string;
		fs::path Path;
		string Issuer;     //RFC2253 one-line DN.
		string SubjectAltName;    //RFC2253 one-line DN.
		string CommonName; //subject CN, empty if absent.
		string Country;			//subject C, empty if absent.
		string Company;		//subject O, empty if absent.
		//string Domain;		//subject CN, empty if absent.
		string Upn;        //SAN otherName 1.3.6.1.4.1.311.20.2.3 (ms UPN), empty if absent.
		string Email;      //SAN rfc822Name, empty if absent.
		TimePoint Expiration; //notAfter.
	};

	struct Γ CryptoSettings final{
		CryptoSettings( str settingsPath, optional<sv> defaultFileName )ι;
		CryptoSettings( const jobject& settings, optional<sv> defaultFileName )ι;
		α CreateDirectories()Ε->void;

		struct PublicKeyPath{
			PublicKeyPath( const jobject& o, optional<sv> defaultFileName )ι;
			α Γ Value(SL sl)Ε->const struct Crypto::PublicKey&;
			fs::path Path;
		private:
			mutable optional<Crypto::PublicKey> _value;
		};

		PrivateKeySettings PrivateKey;
		Certificate Certificate;
		PublicKeyPath PublicKey;
		fs::path DhPath;

/*		fs::path CertPath;
		fs::path PrivateKeyPath;

		string Passcode;
		string AltName;
		string Company;
		string Country;
		string Domain;
*/
	};
}
#undef Φ