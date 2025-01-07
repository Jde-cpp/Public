#include <jde/opc/UM.h>
//#include "../../../Framework/source/um/UM.h"
//#include "../../../Framework/source/io/ServerSink.h"
//#include "../../../Framework/source/io/proto/messages.pb.h"
#include <jde/opc/uatypes/UAClient.h>
#include <jde/ql/IQL.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/app/client/appClient.h>

#define let const auto

namespace Jde::Opc{
	static sp<LogTag> _logTag = Logging::Tag( "iot.um" );

	boost::concurrent_flat_map<SessionPK,flat_map<OpcNK,tuple<string,string>>> _sessions; //loginName,password
	AuthenticateAwait::AuthenticateAwait( str loginName, str password, str opcNK, str endpoint, bool isSocket, SL sl )ι:
		base{ sl }, _loginName{ loginName }, _password{ password }, _opcNK{ opcNK }, _endpoint{ endpoint }, _isSocket{ isSocket }{}

	α AuthenticateAwait::Execute()ι->ConnectAwait::Task{
		try{
			optional<bool> authenticated{};
			_sessions.cvisit_while( [this, &authenticated]( let& sessionMap )ι{
				let& map = sessionMap.second;
				if( auto p = map.find(_opcNK); p!=map.end() && get<0>(p->second)==_loginName )
					authenticated = get<1>(p->second)==_password;
				return !authenticated.has_value();
			});
			if( !authenticated.value_or(false) )
				co_await UAClient::GetClient( _opcNK, _loginName, _password );
			CheckProvider();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AuthenticateAwait::CheckProvider()ι->TAwait<Access::ProviderPK>::Task{
		try{
			let providerPK = co_await ProviderSelectAwait{ _opcNK };
			THROW_IF( providerPK==0, "Provider not found for '{}'.", _opcNK );
			AddSession( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AuthenticateAwait::AddSession( Access::ProviderPK providerPK )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task{
		try{
			auto sessionInfo = co_await App::Client::AddSession( _opcNK, _loginName, providerPK, _endpoint, false );
			flat_map<OpcNK,tuple<string,string>> init{ {_opcNK,{_loginName,_password}} };
			std::pair<SessionPK, flat_map<OpcNK,tuple<string,string>>> init2 = make_pair(sessionInfo.session_id(), init);
			_sessions.emplace_or_visit( init2,
				[this]( auto& sessionMap )ι{
					sessionMap.second.try_emplace( _opcNK, make_tuple(_loginName,_password) );
			});
			Resume( move(sessionInfo) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderSelectAwait::Select()ι->TAwait<jobject>::Task{
		try{
			let query = Ƒ( "provider(target:\"{}\", providerTypeId:{}){{ id }}", _opcId, (uint8)Access::EProviderType::OpcServer );
			let j = co_await *App::Client::QLServer()->QueryObject(query, App::Client::AppServiceUserPK() );
			let providerId = Json::FindNumber<Access::ProviderPK>( j, "id" ).value_or(0);
			ResumeScaler( providerId );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderCreatePurgeAwait::Execute( OpcPK opcPK )ι->OpcServerAwait::Task{
		try{
			auto server = co_await OpcServerAwait{ opcPK, true };
			THROW_IF( server.empty(), "[{}]Could not find OpcServer", opcPK );
			if( _insert )
				Insert( move(server.front().Target) );
			else
				Purge( move(server.front().Target) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Insert( str target )ι->TAwait<jobject>::Task{
		let q = Jde::format( "createProvider( target:\"{}\", providerType:\"OpcServer\" ){{id}}", target );
		try{
			let j = co_await *App::Client::QLServer()->QueryObject( q, App::Client::AppServiceUserPK() );
			let newPK = QL::AsId<Access::ProviderPK>( j );
			ResumeScaler( newPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α ProviderCreatePurgeAwait::Purge( str target )ι->ProviderSelectAwait::Task{
		try{
			let providerPK = co_await ProviderSelectAwait{ target };
			Purge( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Purge( Access::ProviderPK providerPK )ι->TAwait<jvalue>::Task{
		let q = Ƒ( "purgeProvider( id:{} )", providerPK );
		try{
			co_await *App::Client::QLServer()->Query( q, App::Client::AppServiceUserPK() );
			ResumeScaler( providerPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ProviderCreatePurgeAwait::Suspend()ι->void{
		if( _opcKey.IsPrimary() )
			Execute( _opcKey.PK() );
		else if( _insert )
			Insert( _opcKey.NK() );
		else
			Purge( _opcKey.NK() );
	}
}
namespace Jde{
	α Opc::Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>{
		string loginName, password;
		_sessions.cvisit( sessionId, [&loginName, &password, &opcId]( let& sessionMap )ι{
			if( auto p = sessionMap.second.find(opcId); p!=sessionMap.second.end() ){
				loginName = get<0>(p->second);
				password = get<1>(p->second);
			}
		});
		return make_tuple( loginName, password );
	}
}