#include "HttpRequestAwait.h"
#include <jde/ql/IQL.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/Node.h>
#include <jde/opc/uatypes/Value.h>
#include "StartupAwait.h"
#include "UAClient.h"
#include "async/ReadAwait.h"
#include "async/Write.h"
#include "async/SessionAwait.h"
#include "auth/PasswordAwait.h"
#include "auth/UM.h"
#include "uatypes/Browse.h"
#include "uatypes/UAClientException.h"

#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ ELogTags::HttpServerRead };
	HttpRequestAwait::HttpRequestAwait( HttpRequest&& req, SL sl )ι:
		base{ move(req), sl }
	{}

	α HttpRequestAwait::await_ready()ι->bool{
		if( _request.IsGet("/ErrorCodes") ){
			vector<StatusCode> scs;
			string scsString = _request["scs"];
			auto strings = Str::Split( scsString );
			jarray j;
			for( let s : strings ){
				let sc = To<StatusCode>( s );
				j.push_back( {{"sc", sc},{"message", UAException::Message(sc)}} );
			}
			_readyResult = mu<jobject>( jobject{{"errorCodes", j}} );
		}
		return _readyResult!=nullptr;
	}
	α HttpRequestAwait::ParseNodes()ε->tuple<flat_set<ExNodeId>,jarray>{
		auto& nodeJson = _request["nodes"];
		auto jNodes = Json::AsArray( Json::ParseValue(move(nodeJson)) );
		flat_set<ExNodeId> nodes;
		for( let& node : jNodes )
			nodes.emplace( Json::AsObject(node) );
		if( nodes.empty() )
			throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "empty nodes" };
		return make_tuple( nodes, move(jNodes) );
	}

	α HttpRequestAwait::ResumeSnapshots( flat_map<ExNodeId, Value>&& results, jarray&& j )ι->void{
		for( let& [nodeId, value] : results )
			j.push_back( jobject{{"node", nodeId.ToJson()}, {"value", value.ToJson()}} );
		Resume( {jobject{{"snapshots", j}}, move(_request)} );
	}

	α HttpRequestAwait::Browse()ι->TAwait<jobject>::Task{
		try{
			let snapshot = ToIV( _request["snapshot"] )=="true";
			_request.LogRead( Ƒ("BrowseObjectsFolder snapshot: {}", snapshot) );
			auto j = co_await ObjectsFolderAwait{ ExNodeId{_request.Params()}, snapshot, move(_client) };
			Resume( {move(j), move(_request)} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α HttpRequestAwait::SnapshotRead( bool write )ι->TAwait<flat_map<ExNodeId, Value>>::Task{
		try{
			auto [nodes, jNodes] = ParseNodes();
			auto results = co_await ReadAwait{ nodes, _client };
			if( find_if( results, []( let& pair )->bool{ return pair.second.hasStatus && pair.second.status==UA_STATUSCODE_BADSESSIONIDINVALID; } )!=results.end() ) {
				throw RestException<http::status::failed_dependency>{ SRCE_CUR, move(_request), "Opc Server session invalid" };
				//co_await AwaitSessionActivation( _client );
				//results = ( co_await Read::SendRequest(nodes, _client) ).UP<flat_map<ExNodeId, Value>>();
			}
			if( !write )
				ResumeSnapshots( move(results), jarray{} );
			else
				SnapshotWrite( move(nodes), move(results), move(jNodes) );
		}
		catch( exception& e ){
			ResumeExp( RestException<http::status::internal_server_error>{SRCE_CUR, move(_request), "SnapshotRead error: {}", e.what()} );
		}
	}
	α HttpRequestAwait::SnapshotWrite( flat_set<ExNodeId>&& nodes, flat_map<ExNodeId, Value>&& values, jarray&& jNodes )ι->TAwait<flat_map<ExNodeId,UA_WriteResponse>>::Task{
		try{
			jarray jValues = Json::AsArray( Json::ParseValue(move(_request["values"])) );
			if( jNodes.size()!=jValues.size() )
				throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "Invalid json: nodes.size={} values.size={}", nodes.size(), jValues.size() };
			//flat_map<ExNodeId, Value> values;
			for( uint i=0; i<jNodes.size(); ++i ){
				ExNodeId node{  Json::AsObject(jNodes[i]) };
				if( auto existingValue = values.find(node); existingValue!=values.end() ){
					THROW_IF( existingValue->second.status, "Node {} has an error: {}.", serialize(node.ToJson()), UAException{existingValue->second.status}.ClientMessage() );
					existingValue->second.Set( jValues.at(i) );
					values.emplace( move(node), existingValue->second );
				}
				else
					throw RestException<http::status::bad_request>( SRCE_CUR, move(_request), "Node {} not found.", serialize(node.ToJson()) );
			}
			auto writeResults = co_await WriteAwait{ move(values), _client };
			flat_set<ExNodeId> successNodes;
			jarray array;
			for( auto& [nodeId, response] : writeResults ){
				jarray j;
				bool error{};
				for( uint i=0; i<response.resultsSize;++i ){
					error = error || response.results[i];
					j.push_back( response.results[i] );
				}
				if( error )
					array.push_back( jobject{{"node", nodeId.ToJson()}, {"sc", j}} );
				else
					successNodes.insert( nodeId );
				UA_WriteResponse_clear( &response );
			}
			if( successNodes.empty() )
				Resume( {jobject{{"snapshots", array}}, move(_request)} );
			else
				SnapshotRead();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α HttpRequestAwait::CoHandleRequest( ServerCnnctnNK&& opcId )ι->ConnectAwait::Task{
		let& target = _request.Target();
		optional<Credential> cred;
		if( _request.SessionId() )
			cred = GetCredential( _request.SessionId(), opcId );
		try{
			_client = co_await UAClient::GetClient( move(opcId), cred.value_or(Gateway::Credential{}) );
			if( _request.IsGet() ){
				if( target=="/browseObjectsFolder" )
					Browse();
				else if( target=="/snapshot" )
					SnapshotRead();
				else if( target=="/write" )
					SnapshotRead( true );
				else
					throw RestException<http::status::not_found>{ SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() };
			}
			else if( _request.IsPost() )
				throw RestException<http::status::not_found>{ SRCE_CUR, move(_request), "Post not supported for target '{}'", target };
			else
				throw RestException<http::status::forbidden>{ SRCE_CUR, move(_request), "Only get/post verb is supported for target '{}'", target };
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}


	α HttpRequestAwait::Login( str endpoint )ι->TAwait<Web::FromServer::SessionInfo>::Task{
		try{
			let body = _request.Body();
			auto domain = Json::FindString( body, "opc" );
			if( !domain )
				throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "opc server not specified" };
			auto user = Json::FindString( body, "user" );
			if( !user )
				throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "user not specified" };
			auto password = Json::AsString( body, "password" );
			_request.LogRead( Ƒ("Login - opc: {}, user: {}", *domain, *user) );
			let sessionInfo = co_await PasswordAwait{ move(*user), move(password), move(*domain), endpoint, false };
			_request.SessionInfo->SessionId = sessionInfo.session_id();
			_request.SessionInfo->IsInitialRequest = true;
			Resume( move(_request) );
		}
		catch( RestException<http::status::bad_request>& e ){
			ResumeExp( move(e) );
		}
		catch( IException& e ){
			ResumeExp( RestException<http::status::unauthorized>(move(e), move(_request)) );
		}
	}
	α HttpRequestAwait::Logout()ι->TAwait<jvalue>::Task{
		Gateway::Logout( _request.SessionId() );
		Sessions::Remove( _request.SessionId() );
		try{
			auto appClient = AppClient();
			co_await *(appClient->QLServer()->Query(Ƒ( "purgeSession(id:\"{:x}\")", _request.SessionId() ), appClient->UserPK()) );
			Resume( move(_request) );
		}
		catch( IException& e )
		{}
	}

	α HttpRequestAwait::Suspend()ι->void{
		up<IException> pException;
 		if( _request.IsPost("/login") ) //used with user/password on Opc Server.
			Login( _request.UserEndpoint.address().to_string() );
		else if( _request.IsPost("/logout") )
			Logout();
		else{
			if( auto opc = _request["opc"]; opc.size() )
				CoHandleRequest( move(opc) );
			else if( _request.Target().size() ){
				_request.LogRead();
				RestException<http::status::not_found> e{ SRCE_CUR, move(_request), "Unknown target '{}'", _request.Target() };
				ResumeExp( RestException<http::status::not_found>(move(e)) );
			}
		}
	}

	α HttpRequestAwait::await_resume()ε->HttpTaskResult{
		if( auto e = Promise() ? Promise()->MoveExp() : nullptr; e ){
			auto pRest = dynamic_cast<IRestException*>( e.get() );
			if( pRest )
				pRest->Throw();
			else
				throw RestException<>{ move(*e), move(_request) };
		}
		return _readyResult
			? HttpTaskResult{ move(*_readyResult), move(_request) }
			: Promise()->Value() ? move( *Promise()->Value() ) : HttpTaskResult{ jobject{}, move(_request) };
	}
}