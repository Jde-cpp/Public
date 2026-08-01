#include <filesystem>
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

	Ω defaultCommonName()ι->string{
		string cn = Settings::FindString("/instanceName").value_or( string{Process::ProductName()} );
		if constexpr( _debug )
			cn+= ".debug";
		cn+= ".webServer";
		return cn;
	}

	//certInstance is the OPC Target (settable via createServerConnection) and fileName the subject CN - both become
	//part of a file name, so separators must not survive.  Substituted, not rejected: getPath's callers are noexcept.
	Ω safeComponent( sv x )ι->string{
		string y{ x };
		for( auto& c : y ){
			if( c=='/' || c=='\\' || c==':' || c=='*' || c=='?' || c=='"' || c=='<' || c=='>' || c=='|' || (unsigned char)c<0x20 )
				c = '_';
		}
		return y;
	}

	Ω getPath( const jobject& settings, str jpath, fs::path subDir, sv fileName, sv certInstance={} )ι->fs::path{
		auto fqFileName = [certInstance]( fs::path base )ι->fs::path{
			if( certInstance.size() )
				base += Ƒ( ".{}.pem", safeComponent(certInstance) );
			return base;
		};

		auto filePath = Json::FindString( settings, jpath );
		if( filePath )
			return fqFileName( *filePath );//an operator-supplied whole path - its separators are meant.

		auto productName = Json::FindString( settings, "productName" ).value_or( string{Process::ProductName()} );
		auto parent = Process::ProgramDataFolder()/Process::CompanyRootDir()/safeComponent(productName)/"ssl";
		if( !subDir.empty() )
			parent /= subDir;
		parent /= safeComponent( fileName );
		if( certInstance.empty() )
			parent += ".pem";
		return fqFileName( parent );
	}

	PublicKey::PublicKey( fs::path path, SL sl )ε:
		PublicKey{ Crypto::ReadPublicKey(path, sl) }
	{}

	Certificate::Certificate( const jobject& settings, sv certInstance )ι:
		CommonName{ Json::FindString(settings, "commonName").value_or(string{defaultCommonName()}) },
		FileStem{ Json::FindString(settings, "fileName").value_or(CommonName) },
		Path{ getPath(settings, "path", "certs", FileStem, certInstance) },
		SubjectAltName{ Json::FindString(settings, "subjectAltName").value_or("") },
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
	Ω ipString( const ASN1_OCTET_STRING* s )ι->string{
		let n = (uint)::ASN1_STRING_length( s );
		let p = ::ASN1_STRING_get0_data( s );
		if( n==4 )
			return Ƒ( "{}.{}.{}.{}", p[0], p[1], p[2], p[3] );
		string y;//ipv6 - colon-separated hextets.
		for( uint i=0; i+1<n; i+=2 )
			y += Ƒ( "{}{:x}", y.empty() ? "" : ":", (uint)((p[i]<<8)|p[i+1]) );
		return y;
	}
	//renders one SAN entry back into openssl config syntax; empty for types we can't round-trip, which are then dropped.
	Ω sanEntry( const GENERAL_NAME& gn )ι->string{
		switch( gn.type ){
		case GEN_EMAIL: return "email:"+asn1String( gn.d.rfc822Name );
		case GEN_DNS: return "DNS:"+asn1String( gn.d.dNSName );
		case GEN_URI: return "URI:"+asn1String( gn.d.uniformResourceIdentifier );
		case GEN_IPADD: return "IP:"+ipString( gn.d.iPAddress );
		case GEN_OTHERNAME:{
			let other = gn.d.otherName;//only the ms UPN form, the one CreateCertificate emits.
			return ::OBJ_obj2nid( other->type_id )==NID_ms_upn && other->value && other->value->type==V_ASN1_UTF8STRING
				? "otherName:"+string{SN_ms_upn}+";UTF8:"+asn1String( other->value->value.utf8string )
				: string{};
		}
		default: return {};
		}
	}
	Certificate::Certificate( std::span<const byte> bytes, SL sl )ε{
		const unsigned char* p = ( const unsigned char* )bytes.data();
		X509Ptr cert{ ::d2i_X509(nullptr, &p, (long)bytes.size()), ::X509_free }; CHECK_NULL( cert.get() );
		Issuer = Crypto::ToString( ::X509_get_issuer_name(cert.get()), sl );
		auto subject = ::X509_get_subject_name( cert.get() );
		DistinguishedName = Crypto::ToString( subject, sl );
		if( let i = ::X509_NAME_get_index_by_NID(subject, NID_commonName, -1); i>=0 )
			CommonName = asn1String( ::X509_NAME_ENTRY_get_data(::X509_NAME_get_entry(subject, i)) );
		up<GENERAL_NAMES, decltype( &::GENERAL_NAMES_free )> sans{ (GENERAL_NAMES*)::X509_get_ext_d2i(cert.get(), NID_subject_alt_name, nullptr, nullptr), ::GENERAL_NAMES_free };//null = no SAN extension, not an error.
		vector<string> sanEntries;
		for( int i = 0; sans && i<sk_GENERAL_NAME_num(sans.get()); ++i ){
			let gn = sk_GENERAL_NAME_value( sans.get(), i );
			if( gn->type==GEN_EMAIL && Email.empty() )
				Email = asn1String( gn->d.rfc822Name );
			else if( gn->type==GEN_OTHERNAME && Upn.empty() ){ //value type checked - the cert is parsed pre-verification, treat as untrusted input.
				let other = gn->d.otherName;
				if( ::OBJ_obj2nid(other->type_id)==NID_ms_upn && other->value && other->value->type==V_ASN1_UTF8STRING )
					Upn = asn1String( other->value->value.utf8string );
			}
			if( auto entry = sanEntry(*gn); entry.size() )
				sanEntries.push_back( move(entry) );
		}
		SubjectAltName = Str::Join( sanEntries );
		tm t{};
		THROW_IFX( ::ASN1_TIME_to_tm(::X509_get0_notAfter(cert.get()), &t)!=1, Crypto::OpenSslException("ASN1_TIME_to_tm failed", sl) );
		using namespace std::chrono;
		Expiration = sys_days{ year{t.tm_year+1900}/month{(unsigned)t.tm_mon+1}/day{(unsigned)t.tm_mday} } + hours{ t.tm_hour } + minutes{ t.tm_min } + seconds{ t.tm_sec };
	}

	//a SAN may carry several entries ("URI:urn:x,DNS:host"); replacing "URI:" across the whole string would hand the
	//caller "urn:x,DNS:host".  Take the one entry.
	α Certificate::SanUri()Ι->string{
		let i = SubjectAltName.starts_with( "URI:" ) ? 0 : SubjectAltName.find( ",URI:" );
		if( i==string::npos )
			return {};
		let start = i + (i ? 5 : 4);
		let end = SubjectAltName.find( ',', start );
		return SubjectAltName.substr( start, end==string::npos ? string::npos : end-start );
	}

	α Certificate::Log( string prefix, SL sl )Ι->void{
		LOGSL( ELogLevel::Information, sl, ELogTags::Crypto,
			"{} {}",
			prefix, ToString()
		);
	}
	α Certificate::ToString()Ι->string{
		return Ƒ(
			"issuer: {}, subject: {}, SubjectAltName: {}, CommonName: {}, UPN: {}, Email: {}, Expiration: {}",
			Issuer, DistinguishedName, SubjectAltName, CommonName, Upn, Email, ToIsoString( Expiration )
		);
	}

	PrivateKeySettings::PrivateKeySettings( const jobject& settings, sv defaultFileName )ι:
		Path{ getPath(settings, "path", "private", defaultFileName) },
		Passcode{ Json::FindString(settings, "passcode").value_or("") }
	{}

	CryptoSettings::PublicKeyPath::PublicKeyPath( const jobject& settings, sv defaultFileName )ι:
		Path{ getPath(settings, "path", "public", defaultFileName) }
	{}

	α CryptoSettings::PublicKeyPath::Value( SL sl )Ε->const struct PublicKey&{
		if( !_value )
			_value = Crypto::PublicKey{ Path, sl };
		return *_value;
	}

	CryptoSettings::CryptoSettings( str settingsPath )ι:
		CryptoSettings{ Settings::FindDefaultObject(settingsPath) }
	{}

	CryptoSettings::CryptoSettings( const jobject& settings, sv certInstance )ι:
		Certificate{ Json::FindDefaultObject(settings, "certificate"), certInstance },
		PrivateKey{ Json::FindDefaultObject(settings, "privateKey"), Certificate.FileStem },
		PublicKey{ Json::FindDefaultObject(settings, "publicKey"), Certificate.FileStem },
		DhPath{ getPath(settings, "dh", "", "dh") }
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