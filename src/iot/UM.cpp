#include <jde/iot/UM.h>
#include "../../../Framework/source/um/UM.h"
#include "../../../Framework/source/io/ServerSink.h"
#include "../../../Framework/source/io/proto/messages.pb.h"
#include <jde/iot/uatypes/UAClient.h>
#include <jde/io/Json.h>

#define var const auto

namespace Jde::Iot{
	static sp<LogTag> _logTag = Logging::Tag( "iot.um" );

	boost::concurrent_flat_map<SessionPK,flat_map<OpcNK,tuple<string,string>>> _sessions; //loginName,password
	α Authenticate( string loginName, string password, OpcNK opcId, HCoroutine h )ι->Task{
		try{
			optional<bool> authenticated{};
			_sessions.cvisit_while( [&loginName, &password, &opcId, &authenticated]( var& sessionMap )ι{
				var& map = sessionMap.second;
				if( auto p = map.find(opcId); p!=map.end() && get<0>(p->second)==loginName )
					authenticated = get<1>(p->second)==password;
				return !authenticated.has_value();
			});
			if( !authenticated.value_or(false) )
				( co_await UAClient::GetClient(opcId, loginName, password) ).SP<UAClient>();
			var providerId = await( ProviderPK, Logging::Server::GraphQL(opcId) );
			var sessionId = await( SessionPK, Logging::Server::AddLogin(opcId, loginName, providerId) );
			flat_map<OpcNK,tuple<string,string>> init{ {opcId,{loginName,password}} };
			std::pair<SessionPK, flat_map<OpcNK,tuple<string,string>>> init2 = make_pair(sessionId, init);
			_sessions.emplace_or_visit( init2, 
  			[&loginName, &password, &opcId]( auto& sessionMap )ι{
					sessionMap.second.try_emplace( opcId, make_tuple(loginName,password) );
			});
			Resume( mu<SessionPK>(sessionId), move(h) );
		} 
		catch( IException& e ){
			Resume( move(e), move(h) );
		}
	}
	AuthenticateAwait::AuthenticateAwait( str loginName, str password, str opcId, SL sl )ι:
		AsyncAwait{ [&](auto h){ return Authenticate(loginName, password, opcId, move(h)); }, sl, "Authenticate" }
	{}

	α LoadProvider( str opcId, HCoroutine h )ι->Task{
		try{
			auto h2 = move(h);
			h = move(h2);
			var query = Jde::format( "query{{ provider(filter:{{target:{{ eq:\"{}\"}}, providerTypeId:{{ eq:{}}}}}){{ id }} }}", opcId, (uint8)Jde::UM::EProviderType::OpcServer );
			var j = await( json, Logging::Server::GraphQL(query) );
			var providerId = Json::Getε<json>( j, "data" ).is_null() ? 0 : Json::Getε<ProviderPK>( j, {"data","provider","id"} );
			Resume( mu<ProviderPK>(providerId), move(h) );
		}
		catch( IException& e ){
			Resume( move(e), move(h) );
		}
	}
	ProviderAwait::ProviderAwait( str opcId, SL sl )ι:
		AsyncAwait{ [&](auto h){ LoadProvider(opcId, move(h)); }, sl, "Provider" }
	{}

	α InsertProvider( variant<OpcPK,OpcNK> pkNK, HCoroutine h )ι->Task{
		try{
			auto opcNK = pkNK.index()==1 ? get<OpcNK>( move(pkNK) ) : string{};
			if( pkNK.index()==0 ){
				auto server = ( co_await OpcServer::Select(get<OpcPK>(pkNK), true) ).UP<OpcServer>(); THROW_IF( !server, "Could not find OpcServer with id='{}'", get<uint>(pkNK) );
				opcNK = move( server->Target );
			}

			var q = Jde::format( "{{ mutation {{ createProvider(  \"input\": {{\"target\":\"{}\",\"providerType\":\"OpcServer\"}} ){{id}} }} }}", opcNK );
			auto j = await( json, Logging::Server::GraphQL(q) );
			uint newPK = Json::Getε<OpcPK>( j, {"data","provider","id"} );
			Resume( mu<OpcPK>(newPK), move(h) );
		}
		catch( Exception& e ){
			Resume( move(e), move(h) );
		}
	}

	α PurgeProvider( variant<OpcPK,OpcNK> pkNK, HCoroutine h )ι->Task{
		auto opcNK = pkNK.index()==1 ? get<OpcNK>( move(pkNK) ) : string{};
		try{
			if( pkNK.index()==0 ){
				auto server = ( co_await OpcServer::Select(get<OpcPK>(pkNK), true) ).UP<OpcServer>(); THROW_IF( !server, "Could not find OpcServer with id='{}'", get<OpcPK>(pkNK) );
				opcNK = server->Target;
			}
			auto providerPK = await( ProviderPK, ProviderAwait{opcNK} );

			var q = Jde::format( "{{ mutation{{purgeProvider( \"id\":{} ){{rowCount}}}} }}", providerPK );
			auto j = await( json, Logging::Server::GraphQL(q) );
			uint rowCount = Json::Getε<uint>( j, {"data","provider","rowCount"} );
			Resume( move(mu<OpcPK>(rowCount)), move(h) );
		}
		catch( Exception& e ){
			Resume( move(e), move(h) );
		}
	}
	ProviderAwait::ProviderAwait( variant<OpcPK,OpcNK> pkNK, bool insert, SL sl )ι:
		AsyncAwait{ [=,id=move(pkNK)](HCoroutine h){ 
			insert ? InsertProvider(move(id), h) : PurgeProvider(move(id), h);}, 
			sl, "ProviderAwait" }
	{}
}
namespace Jde{
	α Iot::Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>{
		string user, password;
		_sessions.cvisit( sessionId, [&user, &password, &opcId]( var& sessionMap )ι{
			if( auto p = sessionMap.second.find(opcId); p!=sessionMap.second.end() ){
				user = get<0>(p->second);
				password = get<1>(p->second);
			}
		});
		return make_tuple( user, password );
	}
}