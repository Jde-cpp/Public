#include "GatewayQLAwait.h"
#include <jde/ql/QLAwait.h>
#include <jde/app/IApp.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "../async/CallAwait.h"
#include "DataTypeQLAwait.h"
#include "NodeQLAwait.h"
#include "VariableQLAwait.h"
#define let const auto

namespace Jde::Opc::Gateway{

	α IGatewayQLAwait::GetClient( QL::IQLAwaitExe* await )ι->TAwait<sp<UAClient>>::Task{
		try{
			auto session = await->Session(); THROW_IF( !session, "No Session for query" );
			auto opcId = await->Input().As<jstring>( "opc" );
			_client = co_await ConnectAwait{ string{opcId}, *session };
			await->Query();
		}
		catch( exception& e ){
			await->ResumeExp( move(e) );
		}
	}

	Ω needsClient( const QL::Input& q )ι->bool{
		return !q.JTableName().starts_with( "serverConnection" ) && q.JTableName()!="__type" && q.JTableName()!="status";
	}
	α GatewayQLAwait::Test( QL::TableQL& q, QL::Creds executer, SL sl )->up<TAwait<jvalue>>{
		up<TAwait<jvalue>> await;
		if( needsClient(q) )
			await = mu<GatewayQLAwait>( move(q), move(executer), sl );
		else if( q.JsonName=="__type" && !q.Args.contains("name") ){ //why?
			NodeId nodeId{ q.Args };
			q.Args["name"] = nodeId.ToString();
		}
		return await;
	}
	α GatewayQLMAwait::Test( QL::MutationQL& m, QL::Creds executer, SL sl )->up<TAwait<jvalue>>{
		return m.JsonTableName=="variable" ? mu<GatewayQLMAwait>( move(m), move(executer), sl ) : nullptr;
	}

	α GatewayQLAwait::Query()ι->TAwait<jvalue>::Task{
		try{
			jvalue y;
			_query.ReturnRaw = true;
			if( _query.JsonName.starts_with("node") || _query.JsonName.starts_with("variable") )
				y = co_await NodeQLAwait{ move(_query), move(_client), _sl };
			else if( _query.JsonName.starts_with("dataType") )
				y = co_await DataTypeQLAwait{ move(_query), move(_client), _sl };
			else if( _query.JsonName=="serverDescription" )
				y = ServerDescription( move(_query), move(_client) );
			else if( _query.JsonName=="securityPolicyUri" )
				y = SecurityPolicyUri( move(_query), move(_client) );
			else if( _query.JsonName=="securityMode" )
				y = SecurityMode( move(_query), move(_client) );
			else
				throw Exception{ _sl, "Unknown query type: {}", _query.JsonName };
			Resume( move(y) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α GatewayQLMAwait::Query()ι->TAwait<jvalue>::Task{
		try{
			jvalue y;
			// if( m.Type==QL::EMutationQL::Execute )
			// 	results.push_back( co_await JCallAwait(move(m), _request.SessionInfo, _sl) );
			if( _query.DBTable && _query.TableName()=="server_connections" )
				y = co_await QL::QLAwait<>( move(_query), UserPK(), _sl );
			else if( _query.JsonTableName=="variable" ){
				auto session = Session();
				THROW_IF( !session, "No Session for mutation" );
				y = co_await VariableQLAwait{ move(_query), session, _sl };
			}
			else
				throw Exception{ _sl, "Unknown query type: {}", _query.JsonTableName };
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α GatewayQLAwait::ServerDescription( QL::TableQL&& q, sp<UAClient> client )ι->jobject{
		UA_Variant uaAttrib;
		UA_Client_getConnectionAttributeCopy( *client, BrowseName{"serverDescription", 0}, &uaAttrib );
		let desc = ( UA_ApplicationDescription* )uaAttrib.data;
		jobject j;
		if( q.FindColumn("applicationUri") )
			j["applicationUri"] = ToString( desc->applicationUri );
		if( q.FindColumn("productUri") )
			j["productUri"] = ToString( desc->productUri );
		if( q.FindColumn("applicationName") )
			j["applicationName"] = ToString( desc->applicationName.text );
		if( q.FindColumn("applicationType") ){
			sv applicationType;
			switch( desc->applicationType ){
			case UA_ApplicationType::UA_APPLICATIONTYPE_SERVER:
				applicationType = "Server";
				break;
			case UA_ApplicationType::UA_APPLICATIONTYPE_CLIENT:
				applicationType = "Client";
				break;
			case UA_ApplicationType::UA_APPLICATIONTYPE_CLIENTANDSERVER:
				applicationType = "ClientAndServer";
				break;
			case UA_ApplicationType::UA_APPLICATIONTYPE_DISCOVERYSERVER:
				applicationType = "DiscoveryServer";
				break;
			}
			j["applicationType"] = applicationType;
		}
		if( q.FindColumn("gatewayServerUri") )
			j["gatewayServerUri"] = ToString( desc->gatewayServerUri );
		if( q.FindColumn("discoveryProfileUri") )
			j["discoveryProfileUri"] = ToString( desc->discoveryProfileUri );
		if( q.FindColumn("discoveryUrls") ){
			jarray discoveryUrls;
			for( size_t i=0; i<desc->discoveryUrlsSize; ++i )
				discoveryUrls.emplace_back( ToString(desc->discoveryUrls[i]) );
			j["discoveryUrls"] = discoveryUrls;
		}
		UA_Variant_clear( &uaAttrib );
		return q.TransformResult( move(j) );
	}
	α GatewayQLAwait::SecurityPolicyUri( QL::TableQL&& q, sp<UAClient> client )ι->jvalue{
		UA_Variant uaAttrib;
		UA_Client_getConnectionAttributeCopy( *client, BrowseName{"securityPolicyUri", 0}, &uaAttrib );
		string uri = ToString( *(UA_String*)uaAttrib.data );
		UA_Variant_clear( &uaAttrib );
		return q.TransformResult( move(uri) );
	}
	α GatewayQLAwait::SecurityMode( QL::TableQL&& q, sp<UAClient> client )ι->jvalue{
		UA_Variant uaAttrib;
		UA_Client_getConnectionAttributeCopy( *client, BrowseName{"securityMode", 0}, &uaAttrib );
		UA_MessageSecurityMode emode = *( UA_MessageSecurityMode* )uaAttrib.data;
		sv mode;
		switch( emode ){
		case UA_MESSAGESECURITYMODE_INVALID:
			mode = "Invalid";
			break;
		case UA_MESSAGESECURITYMODE_NONE:
			mode = "None";
			break;
		case UA_MESSAGESECURITYMODE_SIGN:
			mode = "Sign";
			break;
		case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
			mode = "SignAndEncrypt";
			break;
		}
		UA_Variant_clear( &uaAttrib );
		return q.TransformResult( string{mode} );
	}
}