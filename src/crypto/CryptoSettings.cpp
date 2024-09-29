#include <jde/crypto/CryptoSettings.h>
#include "../../../Framework/source/Settings.h"

namespace Jde::Crypto{
	using namespace Jde::Settings;

	α GetPath( str x, fs::path dflt )ι{
		const fs::path defaultPrefix = IApplication::ApplicationDataFolder()/"ssl";
		return Get<fs::path>( x ).value_or( defaultPrefix/dflt );
	}

	CryptoSettings::CryptoSettings( str prefix )ι:
		CertPath{ GetPath(prefix+"/certificate", "certs/cert.pem") },
		PrivateKeyPath{ GetPath(prefix+"/privateKey", "private/private.pem") },
		PublicKeyPath{ GetPath(prefix+"/publicKey", "public/public.pem") },
		DhPath{ GetPath(prefix+"/dh", "dh.pem") },
		Passcode{ Get(prefix+"/passcode").value_or("") },
		AltName{ Get(prefix+"/certificateAltName").value_or("DNS:localhost,IP:127.0.0.1") },
		Company{ Get(prefix+"/certficateCompany").value_or("Jde-Cpp") },
		Country{ Get(prefix+"/certficateCountry").value_or("US") },
		Domain{ Get(prefix+"/certficateDomain").value_or("localhost") }
	{}

	α CryptoSettings::CreateDirectories()ι->void{
		fs::create_directories( CertPath.parent_path() );
		fs::create_directories( PrivateKeyPath.parent_path() );
		fs::create_directories( PublicKeyPath.parent_path() );
		fs::create_directories( DhPath.parent_path() );
	}
}