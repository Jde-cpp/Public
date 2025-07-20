#include <jde/crypto/CryptoSettings.h>
#include <jde/framework/settings.h>

namespace Jde::Crypto{
	//using namespace Jde::Settings;

	Ω getPath( const jobject& settings, str jpath, fs::path dflt )ι->fs::path{
		auto filePath = Json::FindString( settings, jpath );
		if( !filePath ){
			auto productName = Json::FindString( settings, "productName" ).value_or( string{Process::ProductName()} );
			filePath = IApplication::ProgramDataFolder()/OSApp::CompanyRootDir()/productName/"ssl"/dflt;
		}
		return fs::path{ *filePath };
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
		AltName{ Json::FindString(settings, "certificateAltName").value_or("DNS:localhost,IP:127.0.0.1") },
		Company{ Json::FindString(settings, "certificateCompany").value_or("Jde-Cpp") },
		Country{ Json::FindString(settings, "certificateCountry").value_or("US") },
		Domain{ Json::FindString(settings, "certificateDomain").value_or("localhost") }
	{}

	α CryptoSettings::CreateDirectories()Ι->void{
		fs::create_directories( CertPath.parent_path() );
		fs::create_directories( PrivateKeyPath.parent_path() );
		fs::create_directories( PublicKeyPath.parent_path() );
		fs::create_directories( DhPath.parent_path() );
	}
}