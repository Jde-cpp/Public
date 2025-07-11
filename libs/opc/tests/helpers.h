#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/db.h>
#include <jde/db/Key.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>

namespace Jde::Opc{
	struct OpcQLHook;

	const static string OpcServerTarget{ "OpcServerTests" };
	struct CreateOpcClientAwait : TAwaitEx<OpcClientPK,QL::QLAwait<jobject>::Task>{
		using base = TAwaitEx<OpcClientPK,QL::QLAwait<jobject>::Task>;
		CreateOpcClientAwait( SRCE )ι:base{ sl }{}
		α Execute()ι->QL::QLAwait<jobject>::Task override;
	};
	α CreateOpcClient()ι->OpcClientPK;

	struct PurgeOpcClientAwait : TAwaitEx<uint,QL::QLAwait<>::Task>{
		PurgeOpcClientAwait( optional<OpcClientPK> pk )ι:_pk{pk}{}
		α Execute()ι->QL::QLAwait<>::Task override;
	private:
		optional<OpcClientPK> _pk;
	};
	α PurgeOpcClient( optional<OpcClientPK> id=nullopt )ι->uint;

	α SelectOpcClient( DB::Key id )ι->jobject;
	α AddHook()ι->void;
	α GetHook()ι->OpcQLHook*;
}