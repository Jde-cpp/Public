#include <jde/iot/UM.h>
#include "../../../Framework/source/um/UM.h"
#include "../../../Framework/source/io/ServerSink.h"
#include "../../../Framework/source/io/proto/messages.pb.h"
#include <jde/iot/uatypes/UAClient.h>

#define var const auto

namespace Jde::Iot{
	boost::concurrent_flat_map<SessionPK,flat_map<string,tuple<string,string>>> _sessions; //opcId,loginName,password
	α Authenticate( string loginName, string password, string opcId, HCoroutine h )ι->Task{
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
			uint providerId; up<json> jProviderId;
			try{
				jProviderId = ( co_await Logging::Server::GraphQL(Jde::format("query{{ provider(filter:{{target:{{ eq:\"{}\"}}, providerTypeId:{{ eq:{}}}){{ id }} }}", opcId, (uint)Jde::UM::EProviderType::OpcServer)) ).UP<json>();
				providerId = (*jProviderId)["data"]["provider"]["id"];
			}
			catch( const nlohmann::json::exception& e ){ 
				THROW( "Provider not found for opcId='{}', provider_type_id='{}'.  result='{}'", opcId, (uint)Jde::UM::EProviderType::OpcServer, jProviderId->dump() );
			}
			var sessionId = *( co_await Logging::Server::AddLogin("opc", loginName, providerId) ).UP<SessionPK>();
			flat_map<string,tuple<string,string>> init{ {opcId,{loginName,password}} };
			std::pair<SessionPK, flat_map<string,tuple<string,string>>> init2 = make_pair(sessionId, init);
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