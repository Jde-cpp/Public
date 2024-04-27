#pragma once
#include "Exports.h"

#define Φ __declspec( dllexport ) auto

namespace Jde::Crypto{
	Φ CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath, str passcode )ε->void;
	Φ CreateCertificate( fs::path outputFile, fs::path privateKeyFile, str passcode, sv altName, sv company, sv country, sv domain )ε->void;
	Φ RsaSign( sv value, sv key )ι->string;
	Φ Verify( const vector<unsigned char>& modulus, const vector<unsigned char>& exponent, str decrypted, str signature )ε->void;
	Φ ReadCertificate( const fs::path& certificate )ε->vector<byte>;
	Φ ReadPrivateKey( const fs::path& privateKeyPath, str passcode={} )ε->vector<byte>;
	Φ WritePrivateKey( const fs::path& path, vector<byte>&& privateKey, str passcode = {} )ε->void;
	Φ WriteCertificate( const fs::path& path, vector<byte>&& certificate )ε->void;
	struct OpenSslException final : Exception{
		template<class... Args>
		OpenSslException( fmt::format_string<Args...> fmt, uint rc, SL sl, Args&&... args )ι:
			Exception{ sl, ELogLevel::Warning, rc, fmt, std::forward<Args>(args)... }
		{}
		static Φ CurrentError()ι->string;
  };
}
#undef Φ