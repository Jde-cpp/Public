#pragma once
#include <jde/ql/ql.h>
#include <jde/ql/QLAwait.h>
#include <jde/web/client/usings.h>
#include <jde/web/server/IWebsocketSession.h>
#include <jde/web/server/Sessions.h>
#include "awaits/ForwardExecutionAwait.h"

namespace Jde::App::Server{
	using namespace Jde::Web::Server;
	//using namespace Jde::Http;
	struct ServerSocketSession : TWebsocketSession<Proto::FromServer::Transmission,Proto::FromClient::Transmission>{
		using base = TWebsocketSession<Proto::FromServer::Transmission,Proto::FromClient::Transmission>;
		ServerSocketSession( sp<RestStream> stream, beast::flat_buffer&& buffer, TRequestType&& request, tcp::endpoint&& userEndpoint, uint32 connectionIndex )ι;
		α AppPK()Ι->AppPK{ return _appPK; }
		α Instance()Ι->const Proto::FromClient::Instance&{ return _instance; }
		α InstancePK()Ι->AppInstancePK{ return _instancePK; }
		α OnRead( Proto::FromClient::Transmission&& transmission )ι->void override;
	private:
		α OnClose()ι->void;
		α GetJwt( Jde::RequestId requestId )ι->TAwait<jobject>::Task;
		α ProcessTransmission( Proto::FromClient::Transmission&& transmission, optional<Jde::UserPK> userPK, optional<RequestId> clientRequestId )ι->void;
		α SharedFromThis()ι->sp<ServerSocketSession>{ return std::dynamic_pointer_cast<ServerSocketSession>(shared_from_this()); }
		//α WriteException( IException&& e, Request )ι->void override{ WriteException( move(e), 0 ); }
		α WriteException( exception&& e, RequestId requestId )ι->void override;
		α WriteException(std::string&&, Jde::RequestId)ι->void override;
		α WriteSubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->void override;
		α WriteSubscription( const jvalue& j, RequestId requestId )ι->void override;
		α WriteComplete( RequestId requestId )ι->void override;

		α AddSession( Proto::FromClient::AddSession addSession, RequestId clientRequestId, SL sl )ι->TAwait<Jde::UserPK>::Task;
		α AddInstance( Proto::FromClient::Instance instance, RequestId requestId )ι->TAwait<sp<Web::Server::SessionInfo>>::Task;
		α Execute( string&& bytes, optional<Jde::UserPK> userPK, RequestId clientRequestId )ι->void;
		α ForwardExecution( Proto::FromClient::ForwardExecution&& clientMsg, bool anonymous, RequestId clientRequestId, SRCE )ι->ForwardExecutionAwait::Task;
		α GraphQL( string&& query, bool returnRaw, RequestId requestId )ι->QL::QLAwait<jvalue>::Task;
		α Schemas()Ι->const vector<sp<DB::AppSchema>>& override;
		α SaveLogEntry( Log::Proto::LogEntryClient logEntry, RequestId requestId )->void;
		α SendAck( uint32 id )ι->void override;
		α SessionInfo( SessionPK sessionId, RequestId requestId )ι->void;
		α SetSessionId( SessionPK sessionId, RequestId requestId )->Web::Server::Sessions::UpsertAwait::Task;

		Proto::FromClient::Instance _instance;
		App::AppPK _appPK{};
		AppInstancePK _instancePK{};
		optional<Jde::UserPK> _userPK{};
		ELogLevel _webLevel{ ELogLevel::NoLog };
		ELogLevel _dbLevel{ ELogLevel::NoLog };
	};
}