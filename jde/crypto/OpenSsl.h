#pragma once
#include "exports.h"
#include "CryptoSettings.h"

#define Φ ΓC auto

namespace Jde::Crypto{
	using Modulus = vector<unsigned char>;
	using Exponent = vector<unsigned char>;
	using Signature = vector<unsigned char>;
	using MD5 = array<byte,16>;
	Φ CalcMd5( sv content )ε->MD5;
	Φ CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath, str passcode )ε->void;
	Φ CreateCertificate( fs::path outputFile, fs::path privateKeyFile, str passcode, sv altName, sv company, sv country, sv domain )ε->void;
	Φ CreateKeyCertificate( const CryptoSettings& settings )ε->void;
	Φ Fingerprint( const Modulus& modulus, const Exponent& exponent )ι->MD5;
	Φ ModulusExponent( const fs::path& publicKey )ε->tuple<Modulus,Exponent>;
	Φ ReadCertificate( const fs::path& certificate )ε->vector<byte>;
	Φ ReadPrivateKey( const fs::path& privateKeyPath, str passcode={} )ε->vector<byte>;
	Φ RsaSign( str content, const fs::path& privateKeyFile )ε->Signature;
	Φ Verify( const Modulus& modulus, const Exponent&, str decrypted, const Signature& signature )ε->void;
	Φ WriteCertificate( const fs::path& path, vector<byte>&& certificate )ε->void;
	Φ WritePrivateKey( const fs::path& path, vector<byte>&& privateKey, str passcode = {} )ε->void;

	struct OpenSslException final : IException{
		template<class... Args>
		OpenSslException( fmt::format_string<Args...> fmt, uint rc, SL sl, Args&&... args )ι:
			IException{ sl, ELogLevel::Warning, rc, fmt, std::forward<Args>(args)... }
		{}
		static Φ CurrentError()ι->string;
		α Move()ι->up<IException>{ return mu<OpenSslException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
  };
}
#undef Φ