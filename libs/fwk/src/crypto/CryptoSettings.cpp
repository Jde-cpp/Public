#include <jde/fwk/crypto/CryptoSettings.h>
#include <jde/fwk/settings.h>

namespace Jde::Crypto{
	Ω getPath( const jobject& settings, str jpath, fs::path dflt )ι->fs::path{
		auto filePath = Json::FindString( settings, jpath );
		if( filePath )
			return fs::path{ *filePath };

		auto productName = Json::FindString( settings, "productName" ).value_or( string{Process::ProductName()} );
		return Process::ProgramDataFolder()/Process::CompanyRootDir()/productName/"ssl"/dflt;
	}

	CryptoSettings::CryptoSettings( str prefix )ι:
		CryptoSettings{ Settings::FindDefaultObject(prefix) }
	{}

	CryptoSettings::CryptoSettings( jobject settings )ι:
		CertPath{ getPath(settings, "certificate", "certs/cert.pem") },
		PrivateKeyPath{ getPath(settings, "privateKey", "private/private.pem") },
		PublicKeyPath{ getPath(settings, "publicKey", "public/public.pem") },
		DhPath{ getPath(settings, "dh", "dh.pem") },
		Passcode{ Json::FindString(settings, "passcode").value_or("") },
		AltName{ Json::FindSVPath(settings, "cert/altName").value_or("DNS:localhost,IP:127.0.0.1") },
		Company{ Json::FindSVPath(settings, "cert/company").value_or("Jde-Cpp") },
		Country{ Json::FindSVPath(settings, "cert/country").value_or("US") },
		Domain{ Json::FindSVPath(settings, "cert/domain").value_or("localhost") }
	{}

	α CryptoSettings::CreateDirectories()Ε->void{
		fs::create_directories( CertPath.parent_path() );
		fs::create_directories( PrivateKeyPath.parent_path() );
		fs::create_directories( PublicKeyPath.parent_path() );
		fs::create_directories( DhPath.parent_path() );
	}
}