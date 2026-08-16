#pragma once
#include "usings.h"
#include <jde/fwk/co/AnyAwait.h>

namespace Jde::Access{
	struct IAdminAcl{
		//Resolves if userPK has admin rights, fails with the denial otherwise. Awaitable (AnyVoidAwait - any coroutine
		//can co_await it) because the AppServer's remote implementation round-trips a query to the authorizing client;
		//Local implementations return a pre-completed AnyCompletedAwait.
		β TestAdmin( str resource, str criteria, UserPK userPK, SRCE )ι->up<AnyVoidAwait> =0;
	};
	struct IAcl : IAdminAcl{
		β Test( str schemaName, str resourceName, ERights rights, UserPK executer, SRCE )ε->void=0;
		β Rights( str schemaName, str resourceName, UserPK executer )ι->ERights=0;
		β UserName( UserPK userPK )ι->string=0;
	};
}