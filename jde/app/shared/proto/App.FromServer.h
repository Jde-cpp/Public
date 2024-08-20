#pragma once
// #include "../Sessions.h"
#include "../exports.h"
#include "../usings.h"


#define var const auto
#define Φ ΓAS α
namespace Jde::DB{ struct IRow; struct ColumnQL;}
namespace Jde::Web::Server{ struct SessionInfo; }
namespace Jde::App::FromServer{
	//namespace FromServerProto = Jde::App::Proto::FromServer;
	Φ Ack( uint32 serverSocketId )ι->Proto::FromServer::Transmission;
	Φ Complete( RequestId requestId )ι->Proto::FromServer::Transmission;
	Φ Exception( const IException& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission;
	Φ Execute( string&& executionResult, RequestId clientRequestId )ι->Proto::FromServer::Transmission;
	Φ ExecuteRequest( RequestId serverRequestId, UserPK userPK, string&& fromClient )ι->Proto::FromServer::Transmission;
	Φ GraphQL( string&& queryResults, RequestId requestId )ι->Proto::FromServer::Transmission;
	Φ SessionInfo( const Web::Server::SessionInfo& session, RequestId requestId )ι->Proto::FromServer::Transmission;
	Φ StatusBroadcast( Proto::FromServer::Status status )ι->Proto::FromServer::Transmission;
	Φ ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status;
	Φ ToTrace( const DB::IRow& row, const vector<DB::ColumnQL>& columns )ι->Proto::FromServer::Trace;
	Φ TraceBroadcast( LogPK id, AppPK appId, AppInstancePK instanceId, const Logging::ExternalMessage& m, const vector<string>& args )ι->Proto::FromServer::Transmission;
}

#undef var
#undef Φ