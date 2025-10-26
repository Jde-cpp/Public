#pragma once
#include "OpenSsl.h"

namespace Jde::Crypto{
	struct Γ CryptoSettings final{
		CryptoSettings( str settingsPrefix )ι;
		CryptoSettings( jobject settings )ι;
		α CreateDirectories()Ε->void;

		 α PublicKey()Ι->const Crypto::PublicKey&{
			if( !_publicKey )
				_publicKey = Crypto::ReadPublicKey( PublicKeyPath );
			return *_publicKey;
		}

		fs::path CertPath;
		fs::path PrivateKeyPath;
		fs::path PublicKeyPath;
		fs::path DhPath;
		string Passcode;
		string AltName;
		string Company;
		string Country;
		string Domain;
	private:
		mutable optional<Crypto::PublicKey> _publicKey;
	};
}