#include "WebServer.h"
#include <jde/fwk/co/Timer.h>
#include <jde/ql/LocalQL.h>
#include <jde/ql/types/FilterQL.h>
#include <jde/app/IApp.h>
#include <jde/web/server/Sessions.h>
#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/app/shared/proto/App.FromClient.h>
#include "LogData.h"
#include "ServerSocketSession.h"

#define let const auto
namespace Jde::App::Server{
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
}
namespace Jde::App{
	using QL::FilterQL;
	concurrent_flat_map<AppInstancePK,sp<Server::ServerSocketSession>> _sessions; //Consider using main class+ql subscriptions
	concurrent_flat_map<AppInstancePK,FilterQL> _logSubscriptions;
	concurrent_flat_map<AppInstancePK,Proto::FromServer::Status> _statuses;
	concurrent_flat_set<AppInstancePK> _statusSubscriptions;

	AppPK _appId;
	AppInstancePK _instancePK;
	atomic<RequestId> _requestId{0};
	sp<QL::LocalQL> _ql;
	sp<Server::RequestHandler> _requestHandler;

	α Server::QLPtr()ι->sp<QL::LocalQL>{ ASSERT( _ql ); return _ql; }
	α Server::QL()ι->QL::LocalQL&{ return *QLPtr(); }
	α Server::SetLocalQL( sp<QL::LocalQL> ql )ι->void{ _ql=move(ql); }
	α Server::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return QL().Schemas(); }

	sp<Server::LocalClient> _appClient = ms<Server::LocalClient>();
	α Server::AppClient()ι->sp<LocalClient>{ return _appClient; }

	α Server::GetAppPK()ι->AppPK{ return _appId; }
	α Server::SetAppPKs( std::tuple<AppPK, AppInstancePK> x )ι->void{ _appId=get<0>(x); _appClient->SetInstancePK(get<1>(x)); }
	//α UpdateStatuses()ι->DurationTimer::Task;
	α Server::StartWebServer( jobject&& settings )ε->void{
		_requestHandler = ms<RequestHandler>( move(settings) );
		Web::Server::Start( _requestHandler );
		Process::AddShutdownFunction( []( bool terminate ){Server::StopWebServer(terminate);} );//TODO move to Web::Server
		//UpdateStatuses();
	}
	α Server::RemoveExisting( str host, PortType port )ι->void{
		_sessions.erase_if( [&host=host,port=port]( auto&& kv ){
			auto& existing = kv.second->Instance();
			return existing.host()==host && existing.web_port()==port;
		});
	}

	α Server::GetJwt( UserPK userPK, string name, string target, string endpoint, SessionPK sessionId, TimePoint expires, string description )ι->Web::Jwt{
		auto requestHandler = _requestHandler;
		THROW_IF( !requestHandler, "No request Handler." );
		return requestHandler->GetJwt( userPK, move(name), move(target), move(endpoint), sessionId, expires, move(description) );
	}

	α Server::StopWebServer( bool terminate )ι->void{
		Web::Server::Stop( move(_requestHandler), terminate );
	}

	//α UpdateStatuses()ι->DurationTimer::Task{
		// while( !Process::ShuttingDown() ){
		// 	Server::BroadcastAppStatus();
		// 	co_await DurationTimer{ 1s };
		// }
	//}

	α TestLogPub( const FilterQL& subscriptionFilter, AppPK /*appId*/, AppInstancePK /*instancePK*/, const Logging::Entry& m )ι->bool{
		bool passesFilter{ true };
		let logTags = ELogTags::Socket | ELogTags::Server | ELogTags::Subscription;
		for( let& [jsonColName, columnFilters] : subscriptionFilter.ColumnFilters ){
			if( jsonColName=="level" )
				passesFilter = FilterQL::Test( underlying(m.Level), columnFilters, logTags );
			else if( jsonColName=="time" )
				passesFilter = FilterQL::Test( m.Time, columnFilters, logTags );
			else if( jsonColName=="message" )
				passesFilter = FilterQL::Test( string{m.Text}, columnFilters, logTags );
			else if( jsonColName=="file" )
				passesFilter = FilterQL::Test( string{m.File()}, columnFilters, logTags );
			else if( jsonColName=="function" )
				passesFilter = FilterQL::Test( string{m.Function()}, columnFilters, logTags );
			else if( jsonColName=="line" )
				passesFilter = FilterQL::Test( m.Line, columnFilters, logTags );
			else if( jsonColName=="user_pk" )
				passesFilter = FilterQL::Test( m.UserPK, columnFilters, logTags );
			// else if( jsonColName=="tags" ) TODO
			// 	passesFilter = FilterQL::Test( m.Tags(), columnFilters, logTags );
			// else if( jsonColName=="args" ) TODO
			// 	passesFilter = FilterQL::Test( m.Args, columnFilters, logTags );
			if( !passesFilter )
				break;
		}
		return passesFilter;
	}

	α Server::BroadcastLogEntry( LogPK id, AppPK logAppPK, AppInstancePK logInstancePK, const Logging::Entry& m, const vector<string>& args )ι->void{
		_logSubscriptions.cvisit_all( [&]( let& kv ){
			if( TestLogPub(kv.second, id, logAppPK, m) ){
				_sessions.visit( kv.first, [&](auto&& kv){
						kv.second->Write( FromServer::TraceBroadcast(id, logAppPK, logInstancePK, m, args) );
				});
			}
		});
	}
	α Server::BroadcastStatus( AppPK appPK, AppInstancePK statusInstancePK, str hostName, Proto::FromClient::Status&& status )ι->void{
		auto value{ FromServer::ToStatus(appPK, statusInstancePK, hostName, move(status)) };
		_statuses.emplace_or_visit( statusInstancePK, value, [&](auto& kv){ kv.second = value; } );
		_statusSubscriptions.visit_all( [&](auto subInstancePK){
			_sessions.visit( subInstancePK, [&](auto&& kv){
				kv.second->Write( FromServer::StatusBroadcast(value) );
			});
		});
	}
	α Server::BroadcastAppStatus()ι->void{
		FromClient::Status( {} );
		BroadcastStatus( GetAppPK(), _instancePK, Process::HostName(), FromClient::ToStatus({}) );
	}
	α Server::FindApplications( str name )ι->vector<Proto::FromClient::Instance>{
		vector<Proto::FromClient::Instance> y;
		_sessions.visit_all( [&]( auto&& kv ){
			auto& session = kv.second;
			if( let instance = session->Instance(); instance.application()==name )
				y.push_back( instance );
		} );
		return y;
	}

	α Server::FindInstance( AppInstancePK instancePK )ι->sp<ServerSocketSession>{
		sp<ServerSocketSession> y;
		_sessions.visit( instancePK, [&]( auto&& kv ){ y=kv.second; } );
		return y;
	}

	α Server::NextRequestId()->RequestId{ return ++_requestId; }
	α Server::Write( AppPK appPK, optional<AppInstancePK> instancePK, Proto::FromServer::Transmission&& msg )ε->void{
		if( !_sessions.visit_while( [&]( auto&& kv ){
			auto& session = kv.second;
			auto appInstPK = session->AppPK()==appPK ? session->InstancePK() : 0;
			let found = appInstPK && appInstPK==instancePK.value_or( appInstPK );
			if( found )
				session->Write( move(msg) );
			return !found;
		}) ){
			THROW( "No session found for appPK:{}, instancePK:{}", appPK, instancePK.value_or(0) );
		}
	}
	α Server::RemoveSession( AppInstancePK instancePK )ι->void{
		_logSubscriptions.erase( instancePK );
		UnsubscribeStatus( instancePK );
		UnsubscribeLogs( instancePK );
		bool erased = _sessions.erase_if( instancePK, [&]( auto&& kv ){
			_statuses.erase( kv.second->InstancePK() );
			return true;
		});
		ForwardExecutionAwait::OnCloseConnection( instancePK );
		TRACET( ELogTags::App, "[{:x}]RemoveSession erased: {}", instancePK, erased );
	}

	α Server::SubscribeLogs( string&& qlText, sp<ServerSocketSession> session )ε->void{
		auto ql = QL::Parse( qlText, Schemas() );
		auto tables = ql.IsQueries() ? move(ql.Queries()) : vector<QL::TableQL>{};
		THROW_IF( tables.size()!=1, "Invalid query, expecting single table" );
		auto table = move( tables.front() );
		THROW_IF( table.JsonName!="logs", "Invalid query, expecting logs query" );
		auto filter = table.Filter();//
		auto traces = Data::LoadEntries( table );//TODO awaitable
		_logSubscriptions.emplace( session->InstancePK(), move(filter) );
		traces.add_values();//Signify end.

		session->LogWrite( "SubscribeLogs existing_count: {}", traces.values_size()-1 );
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_traces() = move( traces );
		session->Write( move(t) );
	}

	α Server::SubscribeStatus( ServerSocketSession& session )ι->void{
		_statusSubscriptions.emplace( session.InstancePK() );
		Proto::FromServer::Transmission t;
		_statuses.visit_all( [&](auto&& kv){
			*t.add_messages()->mutable_status() = kv.second;
		});
		if( t.messages_size() ){
			session.LogWrite( "Writing {} statuses.", t.messages_size() );
			session.Write( move(t) );
		}
	}
	α Server::UnsubscribeLogs( AppInstancePK instancePK )ι->bool{
		return _logSubscriptions.erase( instancePK );
	}
	α Server::UnsubscribeStatus( AppInstancePK instancePK )ι->bool{
		return _statusSubscriptions.erase( instancePK );
	}
}
namespace Jde::App::Server{
	RequestHandler::RequestHandler( jobject&& settings )ι:
		IRequestHandler{ move(settings), Server::AppClient() }
	{}
	α RequestHandler::GetWebsocketSession( sp<RestStream>&& stream, beast::flat_buffer&& buffer, TRequestType req, tcp::endpoint userEndpoint, uint32 connectionIndex )ι->sp<IWebsocketSession>{
		auto session = ms<ServerSocketSession>( move(stream), move(buffer), move(req), move(userEndpoint), connectionIndex );
		_sessions.emplace( session->Id(), session );
		return session;
	}

	α RequestHandler::GetJwt( UserPK userPK, string&& name, string&& target, string&& endpoint, SessionPK sessionId, TimePoint expires, string&& description )ι->Web::Jwt{
		auto publicKey = Crypto::ReadPublicKey( Settings().Crypto().PublicKeyPath );
		return Web::Jwt{ move(publicKey), userPK, move(name), move(target), sessionId, move(endpoint), expires, move(description), Settings().Crypto().PrivateKeyPath };
	}
}