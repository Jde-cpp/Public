#pragma once
#include "../usings.h"
#include <jde/ql/usings.h>


namespace Jde::Crypto{ struct PublicKey; }
namespace Jde::DB{ struct Row; }
namespace Jde::QL{ struct ColumnQL; struct Subscription; }
namespace Jde::Web{ struct Jwt; namespace Server{struct SessionInfo;} }
namespace Jde::App::Proto::FromClient { class Status; }
namespace Jde::App::FromServer{
	α Ack( uint32 serverSocketId )ι->Proto::FromServer::Transmission;
	α Complete( RequestId requestId )ι->Proto::FromServer::Transmission;
	α ConnectionInfo( AppPK appPK, AppInstancePK instancePK, AppConnectionPK connectionPK, RequestId clientRequestId, const Crypto::PublicKey& appServerPublicKey, Web::Server::SessionInfo&& session )ι->Proto::FromServer::Transmission;
	α Exception( const exception& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission;
	α Exception( string&& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission;
	α Execute( string&& executionResult, RequestId clientRequestId )ι->Proto::FromServer::Transmission;
	α ExecuteRequest( RequestId serverRequestId, UserPK userPK, string&& fromClient )ι->Proto::FromServer::Transmission;
	α GraphQL( string&& queryResults, RequestId requestId )ι->Proto::FromServer::Transmission;
	α Jwt( Web::Jwt&& jwt, RequestId requestId )ι->Proto::FromServer::Transmission;
	α LogSubscription( AppPK appPK, AppInstancePK instancePK, const Logging::Entry& e, const QL::Subscription& sub )ι->Proto::FromServer::Transmission;
	α Session( const Web::Server::SessionInfo& session, RequestId requestId )->Proto::FromServer::Transmission;
	α StatusBroadcast( Proto::FromServer::Status status )ι->Proto::FromServer::Transmission;
	α SubscriptionAck( flat_set<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->Proto::FromServer::Transmission;
	α Subscription( string&& s, RequestId requestId )ι->Proto::FromServer::Transmission;
	α ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status;
	α ToTrace( DB::Row&& row, const vector<QL::ColumnQL>& columns )ι->Proto::FromServer::Trace;
}