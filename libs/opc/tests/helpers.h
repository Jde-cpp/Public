#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/db.h>
#include <jde/db/Key.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>

namespace Jde::Opc{
	struct OpcQLHook;

	const static string OpcServerTarget{ "OpcServerTests" };
	struct CreateOpcServerAwait : TAwaitEx<OpcPK,QL::QLAwait<jobject>::Task>{
		using base = TAwaitEx<OpcPK,QL::QLAwait<jobject>::Task>;
		CreateOpcServerAwait( SRCE )ι:base{ sl }{}
		α Execute()ι->QL::QLAwait<jobject>::Task override;
	};
	α CreateOpcServer()ι->OpcPK;

	struct PurgeOpcServerAwait : TAwaitEx<uint,QL::QLAwait<>::Task>{
		PurgeOpcServerAwait( optional<OpcPK> pk )ι:_pk{pk}{}
		α Execute()ι->QL::QLAwait<>::Task override;
	private:
		optional<OpcPK> _pk;
	};
	α PurgeOpcServer( optional<OpcPK> id=nullopt )ι->uint;

	α SelectOpcServer( DB::Key id )ι->jobject;
	α AddHook()ι->void;
	α GetHook()ι->OpcQLHook*;
}