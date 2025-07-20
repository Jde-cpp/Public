#pragma once
//#include "../../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/awaits/ScalerAwait.h>

namespace Jde::DB{ struct Value; }
namespace Jde::Access::Server{
	struct AuthenticateAwait final : TAwait<UserPK>{
		AuthenticateAwait( str loginName, uint providerId, str opcServer, SRCE )ι:TAwait<UserPK>{sl},_loginName{loginName}, _providerId{providerId}, _opcServer{opcServer}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->DB::ScalerAwaitOpt<UserPK::Type>::Task;
	private:
		α InsertUser( str prefix, vector<DB::Value>&& params )->TAwait<UserPK::Type>::Task;
		string _loginName;
		uint _providerId;
		string _opcServer;
	};
}