#include "helpers.h"
#include <jde/framework/settings.h>
#include <jde/opc/OpcQLHook.h>
#include <jde/opc/uatypes/helpers.h>
#include <jde/opc/uatypes/UAException.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include "../src/StartupAwait.h"
#include "../src/auth/OpcServerSession.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	using Gateway::QL;
	constexpr ELogTags _tags{ ELogTags::Test };
	//static Opc::OpcQLHook* _pHook;

	α CreateOpcClientAwait::Execute()ι->QL::QLAwait<jobject>::Task{
		try{
			let certificateUri{ Settings::FindSV("/opc/urn").value_or("urn:open62541.server.application") };
			let url{ Settings::FindSV("/opc/url").value_or( "opc.tcp://127.0.0.1:4840") };
			let create = Ƒ( "mutation createClient( target:'{}', name:'My Test Server', certificateUri:'{}', description:'Test basic functionality', url:'{}', isDefault:false ){{id}}", OpcServerTarget, certificateUri, url );
			let createJson = co_await *QL().QueryObject( Str::Replace(create, '\'', '"'), {UserPK::System}, true, _sl );
			ResumeScaler( Json::AsNumber<OpcClientPK>(createJson, "id") );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α PurgeOpcClientAwait::Execute()ι->QL::QLAwait<>::Task{
		if( !_pk.has_value() )
			_pk = SelectOpcClient( OpcServerTarget )->Id;
		let q = Ƒ( "{{ mutation purgeClient('id':{}) }}", *_pk );
		let result = co_await *QL().Query( Str::Replace(q, '\'', '"'), {UserPK::System}, true, _sl );
		ResumeScaler( 1 );
	}
}

namespace Jde::Opc::Gateway{
	α Tests::CreateOpcClient()ι->OpcClientPK{
		atomic_flag done;
		OpcClientPK y;
		[&]()->CreateOpcClientAwait::Task {
			y = co_await CreateOpcClientAwait();
			done.test_and_set();
			done.notify_one();
		}();
		done.wait( false );
		return y;
	}

	α Tests::PurgeOpcClient( optional<OpcClientPK> pk )ι->uint{
		return BlockAwait<PurgeOpcClientAwait,uint>( PurgeOpcClientAwait{pk} );
	}

	α Tests::SelectOpcClient( DB::Key id )ι->optional<OpcClient>{
		let subQuery = id.IsPrimary() ? Ƒ( "client_id:{{eq:{}}}", id.PK() ) : Ƒ( "target: {{eq:\"{}\"}}", id.NK() );
		let select = Ƒ( "client(filter:{{ {} }}){{ client_id name attributes created updated deleted target description certificateUri isDefault url }}", subQuery );
		auto o = QL().QuerySync<>( select, {UserPK::System} );
		return o.empty() ? optional<OpcClient>{} : OpcClient( move(o) );
	}

	flat_map<string,ETokenType> _userTokens;
	α Tests::AvailableUserTokens( sv url_ )ε->ETokenType{
		const string url{url_};
		if( _userTokens.contains(url) )
			return _userTokens[url];
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
		auto UA_DateTime_now_fake = []( UA_EventLoop* )->UA_DateTime{ return 0x5C8F735D; };
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