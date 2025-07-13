#pragma once

namespace Jde::Opc::Gateway{
	struct UAClient;

	struct CertificateAwait : TAwait<Web::FromServer::SessionInfo>{
		using base = TAwait<Web::FromServer::SessionInfo>;
		CertificateAwait( Crypto::Certificate certificate, string endpoint, bool isSocket, SRCE )ι;
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<sp<UAClient>>::Task;
	private:
		α AddSession( Access::ProviderPK providerPK )ι->TAwait<Web::FromServer::SessionInfo>::Task;
		Crypto::Certificate _certificate; string _endpoint; bool _isSocket;
	};
}