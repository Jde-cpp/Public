#pragma once
#include <jde/crypto/OpenSsl.h>
#include <jde/web/Jwt.h>
#include <jde/framework/coroutine/Await.h>

namespace Jde::Web::Server{
	struct JwtLoginAwait : TAwaitEx<UserPK,TAwait<UserPK>::Task>{
		using base = TAwaitEx<UserPK,TAwait<UserPK>::Task>;
		JwtLoginAwait( Web::Jwt&& _jwt, string endpoint, SRCE )ι:base{ sl }, _jwt{ move(_jwt) }, _endpoint{ endpoint }{}
		α Execute()ι->TAwait<UserPK>::Task;
	private:
		Web::Jwt _jwt; string _endpoint;
	};
}