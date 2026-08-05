#pragma once
#include <span>

typedef struct x509_store_st X509_STORE;

namespace Jde::Crypto{
	//Chain-trust verification: a certificate is trusted iff it chains to a root the OS trusts.
	//Windows: eager snapshot of the current-user ROOT store (certmgr.msc "Trusted Root Certification Authorities") taken at construction; OpenSSL builds the chain itself - no CTL/AuthRoot auto-download or Disallowed list, ROOT store only.
	//Linux: X509_STORE default paths - SSL_CERT_FILE/SSL_CERT_DIR override a custom-built OpenSSL whose OPENSSLDIR points away from /etc/ssl.
	//Thread-safety: X509_STORE is internally refcounted+locked and Verify uses a per-call X509_STORE_CTX, so concurrent Verify/IsTrusted on one instance are safe. Intended pattern is configure-then-verify.
	struct Γ TrustStore final{
		TrustStore( bool loadOsStore=true, SRCE )ε;
		α AddCertificate( std::span<const byte> der, SRCE )ε->void;//extra trust anchor (self-signed peer certs, tests).
		α Verify( std::span<const byte> der, SRCE )Ε->void;//throws OpenSslException with X509_verify_cert_error_string reason+code+depth.
		α IsTrusted( std::span<const byte> der )Ι->bool;
		α CertCount()Ι->uint;//loaded objects (certs+CRLs); lazy hash-dir lookups on Linux can leave this 0 even when Verify works.
		//the store itself, for OpenSSL entry points that take one - SSL_CTX_set1_cert_store, to hand a TLS context these anchors.
		//Non-owning: this TrustStore still frees it, so anything that takes ownership needs an X509_STORE_up_ref first (set1 does
		//that itself; the set_cert_store spelling does not).  Copying the certs out instead would drop Linux's lazy hash-dir
		//lookups, which are lookup methods on the store rather than loaded certificates.
		α Native()Ι->X509_STORE*{ return _store.get(); }
	private:
		up<X509_STORE, void(*)(X509_STORE*)> _store;
	};
}
