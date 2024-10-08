#pragma once
#include "exports.h"
namespace Jde::Crypto{

	struct ΓC CryptoSettings final{
		CryptoSettings( str settingsPrefix )ι;
		α CreateDirectories()ι->void;

		const fs::path CertPath;
		const fs::path PrivateKeyPath;
		const fs::path PublicKeyPath;
		const fs::path DhPath;
		const string Passcode;
		const string AltName;
		const string Company;
		const string Country;
		const string Domain;
	};
}