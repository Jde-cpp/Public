#include <jde/iot/UM.h>
#include "../../../Framework/source/um/UM.h"
//#include "../../../Framework/source/io/ServerSink.h"
//#include "../../../Framework/source/io/proto/messages.pb.h"
#include <jde/iot/uatypes/UAClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/io/Json.h>

#define var const auto

namespace Jde::Iot{
	static sp<LogTag> _logTag = Logging::Tag( "iot.um" );

	boost::concurrent_flat_map<SessionPK,flat_map<OpcNK,tuple<string,string>>> _sessions; //loginName,password
	AuthenticateAwait::AuthenticateAwait( str loginName, str password, str opcNK, str endpoint, bool isSocket, SL sl )ι:
		base{ sl }, _loginName{ loginName }, _password{ password }, _opcNK{ opcNK }, _endpoint{ endpoint }, _isSocket{ isSocket }{}

	α AuthenticateAwait::Execute()ι->Jde::Task{
		try{
			optional<bool> authenticated{};
			_sessions.cvisit_while( [this, &authenticated]( var& sessionMap )ι{
				var& map = sessionMap.second;
				if( auto p = map.find(_opcNK); p!=map.end() && get<0>(p->second)==_loginName )
					authenticated = get<1>(p->second)==_password;
				return !authenticated.has_value();
			});
			if( !authenticated.value_or(false) )
				( co_await UAClient::GetClient(_opcNK, _loginName, _password) ).SP<UAClient>();
			var providerPK = await( ProviderPK, ProviderAwait{_opcNK} ); THROW_IF( providerPK==0, "Provider not found for '{}'.", _opcNK );
			[]( AuthenticateAwait& self, auto providerPK )->Web::Client::ClientSocketAwait<App::Proto::FromServer::SessionInfo>::Task{
				try{
					auto sessionInfo = co_await App::Client::AddSession( self._opcNK, self._loginName, providerPK, self._endpoint, false );
					flat_map<OpcNK,tuple<string,string>> init{ {self._opcNK,{self._loginName,self._password}} };
					std::pair<SessionPK, flat_map<OpcNK,tuple<string,string>>> init2 = make_pair(sessionInfo.session_id(), init);
					_sessions.emplace_or_visit( init2,
						[&self]( auto& sessionMap )ι{
							sessionMap.second.try_emplace( self._opcNK, make_tuple(self._loginName,self._password) );
					});
					self.Resume( move(sessionInfo) );
				}
				catch( IException& e ){
					self.ResumeExp( move(e) );
				}
			}( *this, providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α LoadProvider( str opcId, HCoroutine h )ι->Web::Client::ClientSocketAwait<string>::Task{
		auto h2 = h;
		try{
			var query = Jde::format( "{{ query provider(filter:{{target:{{ eq:\"{}\"}}, providerTypeId:{{ eq:{}}}}}){{ id }} }}", opcId, (uint8)Jde::UM::EProviderType::OpcServer );
			var j = Json::Parse( co_await App::Client::GraphQL(query) );
			var providerId = Json::Getε<json>( j, "data" ).is_null() ? 0 : Json::Getε<ProviderPK>( j, {"data","provider","id"} );
			Resume( mu<ProviderPK>(providerId), h2 );
		}
		catch( IException& e ){
			Resume( move(e), h2 );
		}
	}
	ProviderAwait::ProviderAwait( str opcId, SL sl )ι:
		AsyncAwait{ [&](auto h){ LoadProvider(opcId, h); }, sl, "Provider" }
	{}

	α InsertProvider( variant<OpcPK,OpcNK> pkNK, HCoroutine h )ι->Task{
		auto opcNK = pkNK.index()==1 ? get<OpcNK>( move(pkNK) ) : string{};
		if( pkNK.index()==0 ){
			try{
				auto server = ( co_await OpcServer::Select(get<OpcPK>(pkNK), true) ).UP<OpcServer>(); THROW_IF( !server, "Could not find OpcServer with id='{}'", get<OpcPK>(pkNK) );
				opcNK = move( server->Target );
			}
			catch( IException& e ){
				Resume( move(e), move(h) );
			}
		}
		[](auto&& opcNK, auto h)->Web::Client::ClientSocketAwait<string>::Task {
			var q = Jde::format( "{{ mutation createProvider(  \"input\": {{\"target\":\"{}\",\"providerType\":\"OpcServer\"}} ){{id}} }}", opcNK );
			try{
				var j = Json::Parse( co_await App::Client::GraphQL(q) );
				uint newPK = Json::Getε<OpcPK>( j, {"data","provider","id"} );
				Resume( mu<OpcPK>(newPK), h );
			}
			catch( IException& e ){
				Resume( move(e), h );
			}
		}(move(opcNK), h);
	}

	α PurgeProvider( variant<OpcPK,OpcNK> pkNK, HCoroutine h )ι->Task{
		auto opcNK = pkNK.index()==1 ? get<OpcNK>( move(pkNK) ) : string{};
		if( pkNK.index()==0 ){
			try{
				auto server = ( co_await OpcServer::Select(get<OpcPK>(pkNK), true) ).UP<OpcServer>(); THROW_IF( !server, "Could not find OpcServer with id='{}'", get<OpcPK>(pkNK) );
				opcNK = move( server->Target );
			}
			catch( IException& e ){
				Resume( move(e), move(h) );
			}
		}
		ProviderPK providerPK = await( ProviderPK, ProviderAwait{opcNK} );
		[h,providerPK]()->Web::Client::ClientSocketAwait<string>::Task {
			var q = Ƒ( "{{ mutation purgeProvider( \"id\":{} ){{rowCount}} }}", providerPK );
			auto h2 = h;
			try{
				var j = Json::Parse( co_await App::Client::GraphQL(q) );
				uint rowCount = Json::Getε<uint>( j, {"data","provider","rowCount"} );
				Resume( move(mu<OpcPK>(rowCount)), h2 );
			}
			catch( IException& e ){
				Resume( move(e), h2 );
			}
		}();
		co_return;
	}
	ProviderAwait::ProviderAwait( variant<OpcPK,OpcNK> pkNK, bool insert, SL sl )ι:
		AsyncAwait{
			[=,id=move(pkNK)](HCoroutine h){
				if( insert )
					InsertProvider( move(id), h );
				else
					PurgeProvider(move(id), h);
				},
			sl, "ProviderAwait" }
	{}
}
namespace Jde{
	α Iot::Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>{
		string loginName, password;
		_sessions.cvisit( sessionId, [&loginName, &password, &opcId]( var& sessionMap )ι{
			if( auto p = sessionMap.second.find(opcId); p!=sessionMap.second.end() ){
				loginName = get<0>(p->second);
				password = get<1>(p->second);
			}
		});
		return make_tuple( loginName, password );
	}
}