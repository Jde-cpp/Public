#include <jde/fwk/crypto/CryptoSettings.h>
#include <openssl/x509v3.h>
#include <jde/fwk/settings.h>
#include "jde/fwk/io/file.h"
#include "jde/fwk/io/json.h"
#include <jde/fwk/crypto/OpenSsl.h>
#include "OpenSslInternal.h"

#define let const auto
namespace Jde::Crypto{
	using namespace Jde::Crypto::Internal;
/*	Ω defaultDomain()ι->string{//the cert CN doubles as the enrollment identity target, so it must be unique per client - never "localhost".
		return Settings::FindString( "/instanceName" ).value_or( Ƒ("{}{}.{}", Process::AppName(), _debug ? ".Debug" : "", Process::HostName()) );
	}*/
	Ω getPath( const jobject& settings, str jpath, fs::path subDir, sv fileName )ι->fs::path{
		auto filePath = Json::FindString( settings, jpath );
		if( filePath )
			return fs::path{ *filePath };

		auto productName = Json::FindString( settings, "productName" ).value_or( string{Process::ProductName()} );
		return Process::ProgramDataFolder()/Process::CompanyRootDir()/productName/"ssl"/subDir/( string{fileName}+".pem" );
	}

	PublicKey::PublicKey( fs::path path, SL sl )ε:
		PublicKey{ Crypto::ReadPublicKey(path, sl) }
	{}

	Certificate::Certificate( const jobject& settings, optional<sv> defaultFileName )ι:
		Path{ getPath(settings, "path", "certs", defaultFileName.value_or("cert")) },
		SubjectAltName{ Json::FindString(settings, "subjectAltName").value_or("") },
		CommonName{ Json::FindString(settings, "commonName").value_or(string{defaultFileName.value_or("localhost")}) },
		Country{ Json::FindString(settings, "country").value_or("") },
		Company{ Json::FindString(settings, "company").value_or("Jde-Cpp") }{
		//Issuer{ Json::FindString(settings, "issuer").value_or("") },
		//Subject{ Json::FindString(settings, "subject").value_or("") },
		//Upn{ Json::FindString(settings, "upn").value_or("") },
		//Email{ Json::FindString(settings, "email").value_or("") },
		//Expiration{ Json::FindTimePoint(settings, "expiration").value_or(TimePoint{}) }
		ASSERT( CommonName.size() && CommonName!="localhost" );
	}
	Ω asn1String( const ASN1_STRING* s )ι->string{ return {(const char*)::ASN1_STRING_get0_data(s), (uint)::ASN1_STRING_length(s)}; }
	Certificate::Certificate( std::span<const byte> bytes, SL sl )ε{
		const unsigned char* p = ( const unsigned char* )bytes.data();
		X509Ptr cert{ ::d2i_X509(nullptr, &p, (long)bytes.size()), ::X509_free }; CHECK_NULL( cert.get() );
		Issuer = Crypto::ToString( ::X509_get_issuer_name(cert.get()), sl );
		auto subject = ::X509_get_subject_name( cert.get() );
		SubjectAltName = Crypto::ToString( subject, sl );
		if( let i = ::X509_NAME_get_index_by_NID(subject, NID_commonName, -1); i>=0 )
			CommonName = asn1String( ::X509_NAME_ENTRY_get_data(::X509_NAME_get_entry(subject, i)) );
		up<GENERAL_NAMES, decltype( &::GENERAL_NAMES_free )> sans{ (GENERAL_NAMES*)::X509_get_ext_d2i(cert.get(), NID_subject_alt_name, nullptr, nullptr), ::GENERAL_NAMES_free };//null = no SAN extension, not an error.
		for( int i = 0; sans && i<sk_GENERAL_NAME_num(sans.get()); ++i ){
			let gn = sk_GENERAL_NAME_value( sans.get(), i );
			if( gn->type==GEN_EMAIL && Email.empty() )
				Email = asn1String( gn->d.rfc822Name );
			else if( gn->type==GEN_OTHERNAME && Upn.empty() ){ //value type checked - the cert is parsed pre-verification, treat as untrusted input.
				let other = gn->d.otherName;
				if( ::OBJ_obj2nid(other->type_id)==NID_ms_upn && other->value && other->value->type==V_ASN1_UTF8STRING )
					Upn = asn1String( other->value->value.utf8string );
			}
		}
		tm t{};
		THROW_IFX( ::ASN1_TIME_to_tm(::X509_get0_notAfter(cert.get()), &t)!=1, Crypto::OpenSslException("ASN1_TIME_to_tm failed", sl) );
		using namespace std::chrono;
		Expiration = sys_days{ year{t.tm_year+1900}/month{(unsigned)t.tm_mon+1}/day{(unsigned)t.tm_mday} } + hours{ t.tm_hour } + minutes{ t.tm_min } + seconds{ t.tm_sec };
	}

	α Certificate::Log( string prefix, SL sl )Ι->void{
		LOGSL( ELogLevel::Information, sl, ELogTags::Crypto,
			"{} {}",
			prefix, ToString()
		);
	}
	α Certificate::ToString()Ι->string{
		return Ƒ(
			"issuer: {}, SubjectAltName: {}, CommonName: {}, UPN: {}, Email: {}, Expiration: {}",
			Issuer, SubjectAltName, CommonName, Upn, Email, ToIsoString( Expiration )
		);
	}

	PrivateKeySettings::PrivateKeySettings( const jobject& settings, optional<sv> defaultFileName )ι:
		Path{ getPath(settings, "path", "private", defaultFileName.value_or("private")) },
		Passcode{ Json::FindString(settings, "passcode").value_or("") }
	{}

	CryptoSettings::PublicKeyPath::PublicKeyPath( const jobject& settings, optional<sv> defaultFileName )ι:
		Path{ getPath(settings, "path", "public", defaultFileName.value_or("public")) }
	{}

	α CryptoSettings::PublicKeyPath::Value( SL sl )Ε->const struct PublicKey&{
		if( !_value )
			_value = Crypto::PublicKey{ Path, sl };
		return *_value;
	}

	CryptoSettings::CryptoSettings( str settingsPath, optional<sv> defaultFileName )ι:
		CryptoSettings{ Settings::FindDefaultObject(settingsPath), defaultFileName }
	{}

	CryptoSettings::CryptoSettings( const jobject& settings, optional<sv> defaultFileName )ι:
		PrivateKey{ Json::FindDefaultObject(settings, "privateKey"), defaultFileName },
		Certificate{ Json::FindDefaultObject(settings, "certificate"), defaultFileName },
		PublicKey{ Json::FindDefaultObject(settings, "publicKey"), defaultFileName },
		DhPath{ getPath(settings, "dh", ".", "dh.pem") }
/*		AltName{ Json::FindSVPath(settings, "cert/altName").value_or("DNS:localhost,IP:127.0.0.1") },
		Company{ Json::FindSVPath(settings, "cert/company").value_or("Jde-Cpp") },
		Country{ Json::FindSVPath(settings, "cert/country").value_or("US") },
		Domain{ [&]{const auto d = Json::FindSVPath(settings, "cert/domain"); return d ? string{*d} : defaultDomain();}() }//value_or on the optional<sv> would dangle off defaultDomain()'s temporary.
*/
	{}

	α CryptoSettings::CreateDirectories()Ε->void{
		IO::CreateDirectories( Certificate.Path.parent_path() );
		IO::CreateDirectories( PrivateKey.Path.parent_path() );
		IO::CreateDirectories( PublicKey.Path.parent_path() );
		if( !DhPath.empty() )
			IO::CreateDirectories( DhPath.parent_path() );
	}
}