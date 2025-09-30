#pragma once
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/web/Jwt.h>
#include <jde/fwk/co/Await.h>

namespace Jde::Web::Server{
	struct JwtLoginAwait : TAwaitEx<UserPK,TAwait<UserPK>::Task>{
		using base = TAwaitEx<UserPK,TAwait<UserPK>::Task>;
		JwtLoginAwait( Web::Jwt&& _jwt, string endpoint, bool isAppServer, SRCE )ι:base{ sl }, _jwt{ move(_jwt) }, _endpoint{ endpoint }, _isAppServer{ isAppServer }{}
		α Execute()ι->TAwait<UserPK>::Task;
	private:
		Web::Jwt _jwt; string _endpoint; bool _isAppServer;
	};
}