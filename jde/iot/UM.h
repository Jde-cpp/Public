#pragma once

namespace Jde::Iot{
	struct AuthenticateAwait : AsyncAwait{
		AuthenticateAwait( str loginName, str password, str opcId, SRCE )ι;
	};
	Ξ Authenticate( str loginName, str password, str opcId, SRCE )ι{ return AuthenticateAwait{loginName, password, opcId, sl}; }
	α Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>;
}