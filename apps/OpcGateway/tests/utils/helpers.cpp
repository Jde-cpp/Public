#include "helpers.h"
#include <jde/fwk/settings.h>
#include <jde/opc/uatypes/opcHelpers.h>
#include <jde/opc/UAException.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/web/Jwt.h>
#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/app/client/IAppClient.h>
#include "../../src/StartupAwait.h"
#include "../../src/auth/OpcServerSession.h"
#include "../../src/auth/UM.h"
#include "../../src/ql/OpcQLHook.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	using Gateway::QL;
	constexpr ELogTags _tags{ ELogTags::Test };
	//static Opc::OpcQLHook* _pHook;

	α CreateServerCnnctnAwait::Execute()ι->QL::QLAwait<jobject>::Task{
		try{
			let certificateUri{ Settings::FindSV("/opc/urn").value_or("urn:open62541.server.application") };
			let url{ Settings::FindSV("/opc/url").value_or( "opc.tcp://127.0.0.1:4840") };
			let create = Ƒ( "mutation createServerConnection( target:'{}', name:'My Test Server', certificateUri:'{}', description:'Test basic functionality', url:'{}', isDefault:false ){{id}}", OpcServerTarget, certificateUri, url );
			let createJson = co_await *QL().QueryObject( Str::Replace(create, '\'', '"'), {}, {UserPK::System}, true, _sl );
			ResumeScaler( Json::AsNumber<ServerCnnctnPK>(createJson, "id") );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α PurgeServerCnnctnAwait::Execute()ι->QL::QLAwait<>::Task{
		if( !_pk.has_value() )
			_pk = SelectServerCnnctn( OpcServerTarget )->Id;
		let q = Ƒ( "{{ mutation purgeServerConnection('id':{}) }}", *_pk );
		let result = co_await *QL().Query( Str::Replace(q, '\'', '"'), {}, {UserPK::System}, true, _sl );
		ResumeScaler( 1 );
	}
}

namespace Jde::Opc::Gateway{
	α Tests::CreateServerCnnctn()ι->ServerCnnctnPK{
		atomic_flag done;
		ServerCnnctnPK y;
		[&]()->CreateServerCnnctnAwait::Task {
			y = co_await CreateServerCnnctnAwait();
			done.test_and_set();
			done.notify_one();
		}();
		done.wait( false );
		return y;
	}

	α Tests::PurgeServerCnnctn( optional<ServerCnnctnPK> pk )ι->uint{
		return BlockAwait<PurgeServerCnnctnAwait,uint>( PurgeServerCnnctnAwait{pk} );
	}

	α Tests::SelectServerCnnctn( DB::Key id )ι->optional<ServerCnnctn>{
		let subQuery = id.IsPrimary() ? Ƒ( "id:{{eq:{}}}", id.PK() ) : Ƒ( "target: {{eq:\"{}\"}}", id.NK() );
		let select = Ƒ( "serverConnection(filter:{{ {} }}){{ id name attributes created updated deleted target description certificateUri isDefault url }}", subQuery );
		auto o = QL().QuerySync<>( select, {UserPK::System} );
		return o.empty() ? optional<ServerCnnctn>{} : ServerCnnctn( move(o) );
	}

	α Tests::GetConnection( str target )ι->ServerCnnctn{
		auto con = SelectServerCnnctn( {target} );
		if( !con ){
			BlockAwait<ProviderCreatePurgeAwait, Access::ProviderPK>( ProviderCreatePurgeAwait{target, false} );
			let id = BlockAwait<CreateServerCnnctnAwait, ServerCnnctnPK>( CreateServerCnnctnAwait{} );
			con = SelectServerCnnctn( id );
		}
		return *con;
	}

	using Web::Client::ClientHttpAwait;
	using Web::Client::ClientHttpRes;
	α Tests::Query( sv ql, bool raw )ε->jobject{
		try{
			auto res = BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{
				"localhost",
				Ƒ("/graphql?query={}&{}", ql, raw ? "raw" : "" ),
				GatewayPort(),
				{ .Authorization=Ƒ("{:x}", AppClient()->SessionId()), .Verb=http::verb::get, .IsSsl=false }
			});
			return res.Json();
		}
		catch( exception& e ){
		}
		return {};
	}

	flat_map<string,ETokenType> _userTokens;
	α Tests::AvailableUserTokens( sv url_ )ε->ETokenType{
		const string url{url_};
		if( _userTokens.contains(url) )
			return _userTokens[url];
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
		auto UA_DateTime_now_fake = []( UA_EventLoop* ) -> UA_DateTime{ return 0x5C8F735D; };
    config->eventLoop->dateTime_now = UA_DateTime_now_fake;
    config->eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    config->tcpReuseAddr = true;
    UA_ClientConfig_setDefault(config);
    UA_EndpointDescription* endpointArray{}; uint endpointArraySize{};
		auto tokens = _userTokens.try_emplace( url ).first;
    UAε( UA_Client_getEndpoints(client, url.c_str(), &endpointArraySize, &endpointArray) ) ;
		for( auto ep : Iterable<UA_EndpointDescription>(endpointArray, endpointArraySize) ){
			for( auto token : Iterable<UA_UserTokenPolicy>(ep.userIdentityTokens, ep.userIdentityTokensSize) )
				tokens->second |= ToTokenType( token.tokenType );
		}
    UA_Array_delete( endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION] );
    UA_Client_delete( client );
		return tokens->second;
	}
}