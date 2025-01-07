#include <jde/app/client/AppClientSubscriptions.h>
#include <jde/app/client/AppClientSocketSession.h>

#define let const auto
namespace Jde::App{
/*
	concurrent_flat_map<RequestId,sp<QL::IListener>> _listeners;

	α Client::OnMessage( string&& j, RequestId requestId )ι->void{

		try{
			if( !_listeners.visit( requestId, [&]( auto p ){ p->second->OnMessage( move(j) );}) )
		}
		catch( const Exception& e ){
			Warning( _tags, "Error processing message '{}'.", e.what() );
		}
		if( listener==_listeners.end() ){
			Warning( _tags, "No listener found for requestId={}.", requestId );
			return;
		}
		listener->second->OnMessage( move(j) );
	}
}
namespace Jde::App::Client{
	α ClientSubscriptionAwait::Execute()ι->Web::Client::ClientSocketAwait<vector<QL::SubscriptionId>>::Task{
		auto requestId = _session->NextRequestId();
		Trace{ _sl, ELogTags::SocketClientWrite, "[{}]Subscribe: '{}'.", requestId, _query.substr(0, Web::Client::MaxLogLength()) };
		co_await Web::Client::ClientSocketAwait<vector<QL::SubscriptionId>>{ Jde::Proto::ToString(FromClient::Subscription(move(_query), requestId)), requestId, _session, _sl };
		_listeners.emplace( requestId, _listener );
		ResumeScaler( requestId );
	}
//TODO!!!
*/
}