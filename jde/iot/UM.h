#pragma once

namespace Jde::Iot{
	struct AuthenticateAwait : AsyncAwait{
		AuthenticateAwait( str loginName, str password, str opcId, SRCE )ι;
	};
	//CRD - Insert/Purge/Select from um_providers table
	struct ProviderAwait : AsyncAwait{
		ProviderAwait( str opcId, SRCE )ι;//select
		ProviderAwait( variant<uint32,string> opcIdTarget, bool insert, SRCE )ι;//insert/purge
	};	
	Ξ Authenticate( str loginName, str password, str opcId, SRCE )ι{ return AuthenticateAwait{loginName, password, opcId, sl}; }
	α Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>;

}