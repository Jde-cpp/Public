#pragma once
#include <jde/framework/coroutine/Await.h>

namespace Jde::Opc{
	struct IotGraphQL;
	const static string OpcServerTarget{ "OpcServerTests" };
	struct CreateOpcServerAwait : TAwaitEx<OpcPK,Task>{ α Execute()ι->Jde::Task override; };
	α CreateOpcServer()ι->OpcPK;

	struct PurgeOpcServerAwait : TAwaitEx<uint,Task>{
		PurgeOpcServerAwait( optional<OpcPK> pk )ι:_pk{pk}{}
		α Execute()ι->Jde::Task override;
	private:
		optional<OpcPK> _pk;
	};
	α PurgeOpcServer( optional<OpcPK> id=nullopt )ι->uint;

	α SelectOpcServer( uint id=0 )ι->jobject;
	α AddHook()ι->void;
	α GetHook()ι->IotGraphQL*;
}