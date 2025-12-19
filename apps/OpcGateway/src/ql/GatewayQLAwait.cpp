#include "GatewayQLAwait.h"
#include <jde/ql/QLAwait.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "../async/CallAwait.h"
#include "DataTypeQLAwait.h"
#include "NodeQLAwait.h"
#include "VariableQLAwait.h"
#define let const auto

namespace Jde::Opc::Gateway{
	GatewayQLAwait::GatewayQLAwait( QL::RequestQL&& q, sp<Web::Server::SessionInfo> session, bool returnRaw, SL sl )ι:
		base{ sl },
		_raw{ returnRaw },
		_queries{ move(q) },
		_session{ move(session) }{
		if( _session->UserPK==0 )
			WARNT( EOpcLogTags::User, "Session has no user." );
	}

	α GatewayQLAwait::NeedsClient( const QL::TableQL& q )Ι->bool{
		return !q.JsonName.starts_with( "serverConnection" ) && q.JsonName!="__type";
	}
	α GatewayQLAwait::OpcClientNK( const QL::TableQL& q )Ι->optional<ServerCnnctnNK>{
		if( !NeedsClient(q) )
			return nullopt;

		auto opcId = q.FindPtr<jstring>( "opc" );
		return opcId ? string{ *opcId } : string{};
	}

	α GatewayQLAwait::Suspend()ι->void{
		for( auto& q : _queries.Queries() ){
			if( auto opcId = OpcClientNK(q); opcId )
				_clients.try_emplace( *opcId, nullptr );
		}
		GetClients();
	}
	α GatewayQLAwait::GetClients()ι->TAwait<sp<UAClient>>::Task{
		try{
			for( auto p = _clients.begin(); p!=_clients.end(); ++p )
				p->second = co_await ConnectAwait{ p->first, *_session, _sl };
			Query();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α GatewayQLAwait::Query()ι->TAwait<jvalue>::Task{
		try{
			if( _queries.IsQueries() ){
				jarray results;
				for( auto& q : _queries.Queries() ){
					q.ReturnRaw = _raw;
					if( !NeedsClient(q) ){
						if( q.JsonName=="__type" && !q.Args.contains("name") ){
							NodeId nodeId{ q.Args };
							q.Args["name"] = nodeId.ToString();
						}
						results.push_back( co_await QL::QLAwait<>(move(q), _session->UserPK, _sl) );
					}else{
						auto client = _clients.at( *OpcClientNK(q) );
						if( q.JsonName.starts_with("node") || q.JsonName.starts_with("variable") )
							results.push_back( co_await NodeQLAwait{move(q), move(client), _sl} );
						else if( q.JsonName.starts_with("dataType") )
							results.push_back( co_await DataTypeQLAwait{move(q), move(client), _sl} );
						else if( q.JsonName=="serverDescription" )
							results.push_back( ServerDescription(move(q), move(client)) );
						else if( q.JsonName=="securityPolicyUri" )
							results.push_back( SecurityPolicyUri(move(client)) );
						else if( q.JsonName=="securityMode" )
							results.push_back( SecurityMode(move(client)) );

						else
							throw Exception{ _sl, "Unknown query type: {}", q.JsonName };
					}
				}
				jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
				Resume( _raw ? move(y) : jobject{{"data", y}} );
			}
			else if( _queries.IsMutation() ){
				jarray results;
				for( auto& m : _queries.Mutations() ){
					m.ReturnRaw = _raw;
					// if( m.Type==QL::EMutationQL::Execute )
					// 	results.push_back( co_await JCallAwait(move(m), _request.SessionInfo, _sl) );
					if( m.DBTable && m.TableName()=="server_connections" )
						results.push_back( co_await QL::QLAwait<>(move(m), _session->UserPK, _sl) );
					else if( m.JsonTableName=="variable" )
						results.push_back( co_await VariableQLAwait{move(m), _session, _sl} );
				}
				jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
				Resume( _raw ? move(y) : jobject{{"data", move(y)}} );
			}
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α GatewayQLAwait::Mutate()ι->TAwait<jvalue>::Task{
		try{
			jarray results;
			for( auto& m : _queries.Mutations() ){
				m.ReturnRaw = _raw;
				if( m.Type==QL::EMutationQL::Execute )
					results.push_back( co_await JCallAwait(move(m), _session, _sl) );
				else if( m.TableName()=="server_connections" )
					results.push_back( co_await QL::QLAwait<>(move(m), _session->UserPK, _sl) );
			}
			jvalue y{ results.size()==1 ? move(results[0]) : jvalue{results} };
			Resume( _raw ? move(y) : jobject{{"data", move(y)}} );
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
		return q.ReturnRaw ? j : jobject{ {"serverDescription", move(j)} };
	}
	α GatewayQLAwait::SecurityPolicyUri( sp<UAClient> client )ι->jvalue{
		UA_Variant uaAttrib;
		UA_Client_getConnectionAttributeCopy( *client, BrowseName{"securityPolicyUri", 0}, &uaAttrib );
		string uri = ToString( *(UA_String*)uaAttrib.data );
		UA_Variant_clear( &uaAttrib );
		return _raw ? jvalue{ uri } : jvalue{ jobject{{"securityPolicyUri", move(uri)}} };
	}
	α GatewayQLAwait::SecurityMode( sp<UAClient> client )ι->jvalue{
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
		return _raw ? jvalue{ mode } : jvalue{ jobject{{"securityMode", mode}} };
	}
}