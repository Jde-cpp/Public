#include "ServerTrust.h"
#include <open62541/plugin/certificategroup_default.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/fwk/crypto/TrustStore.h>

#define let const auto
namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::OpcCrypto };
	namespace{
		//the group's context - owned by the group, freed by clear() when open62541 clears the client config.
		struct Context final{
			Context( Jde::Handle h, string url )ε: Store{false}, Handle{h}, Url{move(url)}{}
			Crypto::TrustStore Store;//anchors only - see the header.
			Jde::Handle Handle;
			string Url;
			uint Anchors{};
			string Rejection;//the last failure;  written by verifyCertificate and read by StateCallback, both on the client's strand.
		};
		α context( const UA_CertificateGroup& g )ι->Context*{ return (Context*)g.context; }

		//C shim in the group vtable (the UATrust::verifyCertificate pattern:  try/catch -> status).
		Ω verifyCertificate( UA_CertificateGroup* g, const UA_ByteString* certificate )ι->UA_StatusCode{
			auto& c = *context( *g );
			try{
				THROW_IF( !certificate || !certificate->length, "the endpoint carries no certificate" );
				c.Store.Verify( std::span<const byte>{(const byte*)certificate->data, certificate->length} );
				c.Rejection.clear();
				return UA_STATUSCODE_GOOD;
			}
			catch( const std::exception& e ){
				c.Rejection = Ƒ( "server certificate for '{}' rejected: {} ({} trusted certificate{} loaded from /access/trustedCertDirs) - add the server's certificate to one of those directories, or set /gateway/verifyServerCertificate=false", c.Url, e.what(), c.Anchors, c.Anchors==1 ? "" : "s" );
				ERRT( _tags, "[{}]{}", hex(c.Handle), c.Rejection );
				return UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
			}
		}
		Ω clear( UA_CertificateGroup* g )ι->void{
			delete context( *g );
			g->context = nullptr;
			UA_NodeId_clear( &g->certificateGroupId );
			g->verifyCertificate = nullptr;
			g->clear = nullptr;
		}
		Ω loadAnchors( Context& c, const vector<fs::path>& dirs, SL sl )ι->void{
			for( let& dir : dirs ){
				try{
					if( !fs::exists(dir) || !fs::is_directory(dir) ){
						WARNT( _tags, "[{}]Trusted certificate directory does not exist: '{}'.", hex(c.Handle), dir.string() );
						continue;
					}
					for( let& entry : fs::directory_iterator(dir) ){
						if( entry.path().extension()!=".pem" && entry.path().extension()!=".crt" )
							continue;
						try{
							c.Store.AddCertificate( Crypto::ReadCertificate(entry.path(), sl), sl );
							++c.Anchors;
						}
						catch( const std::exception& e ){//one unreadable file must not empty the trust list.
							WARNT( _tags, "[{}]Could not load trusted certificate '{}': {}", hex(c.Handle), entry.path().string(), e.what() );
						}
					}
				}
				catch( const fs::filesystem_error& e ){
					WARNT( _tags, "[{}]Could not scan trusted certificate directory '{}': {}", hex(c.Handle), dir.string(), e.what() );
				}
			}
		}
	}

	α ServerTrust::Enabled()ι->bool{ return Settings::FindBool("/gateway/verifyServerCertificate").value_or(true); }

	static std::mutex _overrideMutex;
	static optional<vector<fs::path>> _override;//the test seam - see the header.
	α ServerTrust::OverrideTrustedCertDirs( optional<vector<fs::path>> dirs )ι->void{
		std::lock_guard _{ _overrideMutex };
		_override = move( dirs );
	}
	α ServerTrust::Install( UA_ClientConfig& config, Jde::Handle h, str url, SL sl )ε->void{
		vector<fs::path> dirs;
		{
			std::lock_guard _{ _overrideMutex };
			if( _override )
				dirs = *_override;
		}
		if( dirs.empty() ){
			for( let& dir : Settings::FindStringArray("/access/trustedCertDirs") )
				dirs.emplace_back( dir );
		}
		Install( config, Enabled(), dirs, h, url, sl );
	}
	α ServerTrust::Install( UA_ClientConfig& config, bool verify, const vector<fs::path>& dirs, Jde::Handle h, str url, SL sl )ε->void{
		auto& g = config.certificateVerification;
		if( !verify ){
			UA_CertificateGroup_AcceptAll( &g );//clears whatever was there first.
			WARNT( _tags, "[{}]Server-certificate verification is off (/gateway/verifyServerCertificate) - any certificate '{}' presents is accepted.", hex(h), url );
			return;
		}
		auto c = mu<Context>( h, url );
		loadAnchors( *c, dirs, sl );
		if( g.clear )
			g.clear( &g );//AcceptAll, or an earlier Install.
		g.certificateGroupId = UA_NS0ID( SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP );
		g.logging = config.logging;
		g.verifyCertificate = &verifyCertificate;
		g.clear = &clear;
		g.getTrustList = nullptr; g.setTrustList = nullptr; g.addToTrustList = nullptr; g.removeFromTrustList = nullptr; g.getRejectedList = nullptr; g.getCertificateCrls = nullptr;//as UA_CertificateGroup_AcceptAll leaves them - the client only ever calls verifyCertificate and clear.
		if( c->Anchors )
			INFOT( _tags, "[{}]Verifying '{}'s certificate against {} trusted certificate{} from /access/trustedCertDirs.", hex(h), url, c->Anchors, c->Anchors==1 ? "" : "s" );
		if( !c->Anchors )//not `else`: the log macros expand to their own `if`.
			WARNT( _tags, "[{}]No trusted certificates under /access/trustedCertDirs ({} director{}) - every certificate '{}' presents will be rejected.", hex(h), dirs.size(), dirs.size()==1 ? "y" : "ies", url );
		g.context = c.release();
	}
	α ServerTrust::Rejection( const UA_ClientConfig& config )ι->string{
		let& g = config.certificateVerification;
		return g.verifyCertificate==&verifyCertificate && g.context ? context(g)->Rejection : string{};
	}
	α ServerTrust::AnchorCount( const UA_ClientConfig& config )ι->uint{
		let& g = config.certificateVerification;
		return g.verifyCertificate==&verifyCertificate && g.context ? context(g)->Anchors : 0;
	}
}
