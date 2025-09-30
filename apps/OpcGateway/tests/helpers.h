#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/db/db.h>
#include <jde/db/Key.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>
#include "../src/usings.h"
#include "../src/types/ServerCnnctn.h"

namespace Jde::Opc::Gateway{ enum class ETokenType : uint8; }
namespace Jde::Opc::Gateway::Tests{
	const static string OpcServerTarget{ "OpcServerTests" };
	struct CreateServerCnnctnAwait : TAwaitEx<ServerCnnctnPK,QL::QLAwait<jobject>::Task>{
		using base = TAwaitEx<ServerCnnctnPK,QL::QLAwait<jobject>::Task>;
		CreateServerCnnctnAwait( SRCE )ι:base{ sl }{}
		α Execute()ι->QL::QLAwait<jobject>::Task override;
	};
	α CreateServerCnnctn()ι->ServerCnnctnPK;

	struct PurgeServerCnnctnAwait : TAwaitEx<uint,QL::QLAwait<>::Task>{
		PurgeServerCnnctnAwait( optional<ServerCnnctnPK> pk )ι:_pk{pk}{}
		α Execute()ι->QL::QLAwait<>::Task override;
	private:
		optional<ServerCnnctnPK> _pk;
	};
	α PurgeServerCnnctn( optional<ServerCnnctnPK> id=nullopt )ι->uint;

	α GetConnection( str target )ι->ServerCnnctn;
	α SelectServerCnnctn( DB::Key id )ι->optional<ServerCnnctn>;

	α AvailableUserTokens( sv url )ε->ETokenType;
	α Query( sv ql, bool raw=true )ε->jobject;
}