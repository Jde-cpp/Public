#pragma once
#include <jde/fwk/chrono.h>
#include <jde/web/server/Sessions.h>
#include "../client/proto/Web.FromServer.pb.h"

namespace Jde::Web::Server{
	α ToProto( const Web::Server::SessionInfo& session )ι->FromServer::SessionInfo{
		FromServer::SessionInfo proto;
		*proto.mutable_expiration() = Protobuf::ToTimestamp( Chrono::ToClock<Clock,steady_clock>(session.Expiration) );
		proto.set_session_id( session.SessionId );
		proto.set_user_pk( session.UserPK );
		proto.set_user_endpoint( session.UserEndpoint );
		proto.set_has_socket( session.HasSocket );
		return proto;
	}
}
