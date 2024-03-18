#pragma once
#include "exports.h"

#define Φ ΓC auto

namespace Jde::Crypto{
	Φ CreateKey( const fs::path& publicKeyPath, const fs::path& privateKeyPath )ε->void;
	Φ RsaSign( sv value, sv key )ι->string;
	Φ Verify( const vector<unsigned char>& modulus, const vector<unsigned char>& exponent, str decrypted, str signature )ε->void;
	Φ ReadCertificate( const fs::path& certificate )ε->vector<byte>;
	Φ ReadPrivateKey( const fs::path& privateKeyPath, str passcode={} )ε->vector<byte>;
	struct OpenSslException final : Exception{
		template<class... Args>
		OpenSslException( fmt::format_string<Args...> fmt, uint rc, SL sl, Args&&... args )ι:
			Exception{ sl, ELogLevel::Warning, rc, fmt, std::forward<Args>(args)... }
		{}
		static Φ CurrentError()ι->string;
  };
}
#undef Φ