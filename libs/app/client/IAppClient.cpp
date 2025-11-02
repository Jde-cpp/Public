#include <jde/app/client/IAppClient.h>
#include <jde/fwk/io/proto.h>
#include <jde/ql/IQL.h>
#include <jde/app/log/ProtoLog.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/RemoteLog.h>
#include <jde/app/client/awaits/SocketAwait.h>

#define let const auto
namespace Jde::App::Client{
	α IAppClient::InitLogging( sp<App::Client::IAppClient> client )ι->void{
		App::ProtoLog::Init();
		App::Client::RemoteLog::Init( move(client) );
		Logging::Init();
	}
	α IAppClient::QueryArray( string&& q, jobject variables, bool returnRaw, SL sl )ε->up<TAwait<jarray>>{
		return QLServer()->QueryArray( move(q), move(variables), UserPK(), returnRaw, sl );
	}
	α IAppClient::QueryObject( string&& q, jobject variables, bool returnRaw, SL sl )ε->up<TAwait<jobject>>{
		return QLServer()->QueryObject( move(q), move(variables), UserPK(), returnRaw, sl );
	}
	α IAppClient::QueryValue( string&& q, jobject variables, bool returnRaw, SL sl )ε->up<TAwait<jvalue>>{
		return QLServer()->Query( move(q), move(variables), UserPK(), returnRaw, sl );
	}
	α IAppClient::SessionInfoAwait( SessionPK sessionPK, SL sl )ι->up<TAwait<Web::FromServer::SessionInfo>>{
	 	return mu<Client::SessionInfoAwait>( sessionPK, _session, sl );
	}

	constexpr ELogTags _tags{ ELogTags::SocketClientWrite };
	using Web::Client::ClientSocketAwait;
	α IAppClient::AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, SL sl )ε->ClientSocketAwait<Web::FromServer::SessionInfo>{
		auto p = Session();
		auto requestId = p->NextRequestId();
		TRACESL( "AddSession domain: '{}', loginName: '{}', providerPK: {}, userEndPoint: '{}', isSocket: {}.", domain, loginName, providerPK, userEndPoint, isSocket );
		return ClientSocketAwait<Web::FromServer::SessionInfo>{ FromClient::AddSession(domain, loginName, providerPK, userEndPoint, isSocket, requestId), requestId, p, sl };
	}
	α IAppClient::Jwt( SL sl )ε->ClientSocketAwait<Web::Jwt>{
		auto p = Session();
		auto requestId = p->NextRequestId();
		TRACESL( "Jwt requestId: {}", requestId );
		return ClientSocketAwait<Web::Jwt>{ FromClient::Jwt(requestId), requestId, p, sl };
	}

	α IAppClient::UpdateStatus()ι->void{
		if( auto session = Process::ShuttingDown() ? nullptr : _session; session )
			session->Write( FromClient::Status(StatusDetails()) );
	}

	α IAppClient::CloseSocketSession( SL sl )ι->VoidTask{
		auto session = _session;
		if( !session )
			co_return;
		let tags = ELogTags::Client | ELogTags::Socket;
		LOGSL( ELogLevel::Trace, sl, tags, "ClosingSocketSession" );
		co_await session->Close();
		session = nullptr;
		LOGSL( ELogLevel::Information, sl, tags, "ClosedSocketSession" );
	}
	α IAppClient::Write( vector<Logging::Entry>&& entries )ι->void{
		auto session = _session;
		if( !session )
			return;
		session->Write( FromClient::LogEntries( move(entries) ) );
	}
}