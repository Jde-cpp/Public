#pragma once
#include "AuthAwait.h"

namespace Jde::Opc::Gateway{
	struct UAClient;

	struct CertAwait : AuthAwait<bool>, boost::noncopyable{
		using base = AuthAwait<bool>;
		CertAwait( OpcClientNK opcNK, string endpoint, bool isSocket, SRCE )ι:
			AuthAwait{ Credential{Crypto::PublicKey{}}, move(opcNK), move(endpoint), isSocket, sl }{}

		α Suspend()ι->void override{ Execute(); }
	private:
		α OnSuccess()ι->void override{ ResumeScaler( true ); }
	};
}