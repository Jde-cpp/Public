#pragma once
#include <jde/ql/IQLAwaitExe.h>

namespace Jde::Opc::Gateway{
	//opcSessions{ connection{target} type user{id target name} count } - the live OPC credential cache (auth/OpcServerSession), one row per (connection, credential type, user).
	//No UAClient: reads _sessions directly, so it bypasses GatewayQLAwait's ConnectAwait. user{...} columns beyond id come from AppServer's users table.
	struct OpcSessionsQLAwait final : QL::IQLTableAwaitExe{
		using base = QL::IQLTableAwaitExe;
		OpcSessionsQLAwait( QL::TableQL&& q, QL::Creds&& creds, SRCE )ι:base{ move(q), move(creds), sl }{}
	private:
		α Query()ι->TAwait<jvalue>::Task override;
	};
	//serverConnections{ … opcSessions{count} opcConnections{count} } - grafts live totals onto the DB rows, each child one object {count}: opcSessions = web sessions holding a credential on the target (auth cache), opcConnections = open UAClients (idle drain/disconnect shrink it).
	struct ServerCnnctnSessionsQLAwait final : QL::IQLTableAwaitExe{
		using base = QL::IQLTableAwaitExe;
		ServerCnnctnSessionsQLAwait( QL::TableQL&& q, QL::Creds&& creds, SRCE )ι:base{ move(q), move(creds), sl }{}
		Ω IsApplicable( const QL::TableQL& q )ι->bool{ return q.JsonName.starts_with("serverConnection") && (q.FindTable("opcSessions") || q.FindTable("opcConnections")); }
	private:
		α Query()ι->TAwait<jvalue>::Task override;
	};
}
