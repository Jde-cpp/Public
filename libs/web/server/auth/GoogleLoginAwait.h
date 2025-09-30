#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/web/Jwt.h>
#include <jde/web/client/http/ClientHttpAwait.h>

namespace Jde::Web::Server{
	struct GoogleLoginAwait : TAwaitEx<UserPK,Web::Client::ClientHttpAwait::Task>{
		using base = TAwaitEx<UserPK,Web::Client::ClientHttpAwait::Task>;
		GoogleLoginAwait( Web::Jwt jwt, SRCE ):base{_sl},_jwt{ move(jwt) }{}
	private:
		α Execute()ι->Web::Client::ClientHttpAwait::Task;
		α Authenticate()->TAwait<UserPK>::Task;

		Web::Jwt _jwt;
	};
}