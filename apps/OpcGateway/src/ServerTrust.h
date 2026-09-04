#pragma once
#include <jde/opc/uatypes/Logger.h>

//Server-certificate verification for the gateway's UA clients (MVP "Gateway→OPC-server certificate verification").
//open62541 verifies the server's certificate through UA_ClientConfig::certificateVerification in initSecurityPolicy,
//for every endpoint that carries one - None security mode included - and UA_ClientConfig_setDefault installs AcceptAll
//there, so until this the gateway trusted whatever certificate a server answered with.  Install puts a
//Crypto::TrustStore-backed group in its place, anchored on the certificates under /access/trustedCertDirs - the same
//list the OpcServer trusts client certificates from (UATrust) and the AppServer anchors enrollment on - or AcceptAll
//when /gateway/verifyServerCertificate is false.  Anchors only, no OS root store:  an OPC server's certificate is
//self-signed, so a public CA vouches for nothing here.  Loaded per client, i.e. per connect, so a certificate copied in
//or re-issued after startup is trusted by the next connection without a rescan.
namespace Jde::Opc::Gateway::ServerTrust{
	α Enabled()ι->bool;///gateway/verifyServerCertificate, default true.
	//Before UA_ClientConfig_setDefault, which only fills a null verifyCertificate.  `url` names the server in the log and the rejection.
	α Install( UA_ClientConfig& config, Jde::Handle h, str url, SRCE )ε->void;
	α Install( UA_ClientConfig& config, bool verify, const vector<fs::path>& trustedCertDirs, Jde::Handle h, str url, SRCE )ε->void;//explicit, for tests.
	//Test seam:  the directories the settings-driven Install uses instead of /access/trustedCertDirs, for every client
	//created until nullopt restores the setting.  Jde.Opc.Tests hosts the AppServer in-process, and /access/trustedCertDirs
	//is also its enrollment anchor - overriding the setting itself would refuse the OpcServer's own login before any OPC
	//connect, so the live rejection can only be provoked here.
	α OverrideTrustedCertDirs( optional<vector<fs::path>> dirs )ι->void;
	//Why this client's verifier last rejected a server certificate, "" if it never did or verification is off - the detail
	//StateCallback hands the waiting requests, since the status alone (BadCertificateUntrusted) reads the same as the server
	//rejecting OUR certificate.
	α Rejection( const UA_ClientConfig& config )ι->string;
	α AnchorCount( const UA_ClientConfig& config )ι->uint;//0 when verification is off.
}
