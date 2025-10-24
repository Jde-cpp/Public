#include "HttpRequestAwait.h"
#include <jde/ql/IQL.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>
#include "StartupAwait.h"
#include "UAClient.h"
#include "async/ReadValueAwait.h"
#include "async/SessionAwait.h"
#include "auth/PasswordAwait.h"
#include "auth/UM.h"
#include "ql/GatewayQLAwait.h"
#include "uatypes/Browse.h"

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
				j.push_back( UAException::ToJson(sc, true) );
			}
			_readyResult = mu<jvalue>( jobject{{"errorCodes", j}} );
		}
		return _readyResult!=nullptr;
	}
	α HttpRequestAwait::ParseNodes()ε->tuple<flat_set<NodeId>,jarray>{
		auto& nodeJson = _request["nodes"];
		auto jNodes = Json::AsArray( Json::ParseValue(move(nodeJson)) );
		flat_set<NodeId> nodes;
		for( let& node : jNodes )
			nodes.emplace( Json::AsObject(node) );
		if( nodes.empty() )
			throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "empty nodes" };
		return make_tuple( nodes, move(jNodes) );
	}

	α HttpRequestAwait::ResumeSnapshots( flat_map<NodeId, Value>&& results, jarray&& j )ι->void{
		for( let& [nodeId, value] : results ){
			jobject node = nodeId.ToJson();
			node["value"] = value.ToJson();
			j.push_back( node );
		}
		Resume( {jobject{{"snapshots", j}}, move(_request)} );
	}


	α HttpRequestAwait::CoHandleRequest( ServerCnnctnNK&& opcId )ι->ConnectAwait::Task{
		let& target = _request.Target();
		try{
			_client = co_await ConnectAwait( move(opcId), _request.SessionId(), _request.UserPK(), SRCE_CUR );
			if( _request.IsGet() ){
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


	α HttpRequestAwait::Login( str endpoint )ι->TAwait<optional<Web::FromServer::SessionInfo>>::Task{
		try{
			let body = _request.Body();
			auto domain = Json::FindString( body, "opc" );
			if( !domain )
				throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "opc server not specified" };
			auto user = Json::FindString( body, "user" );
			if( !user )
				throw RestException<http::status::bad_request>{ SRCE_CUR, move(_request), "user not specified" };
			auto password = Json::AsString( body, "password" );
			_request.LogRead( Ƒ("(opc: {}, user: {})", *domain, *user) );
			let sessionInfo = co_await PasswordAwait{ move(*user), move(password), move(*domain), endpoint, false, _request.SessionInfo->SessionId };
			if( sessionInfo ){
				_request.SessionInfo->SessionId = sessionInfo->session_id();
				_request.SessionInfo->IsInitialRequest = true;
			}
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
			co_await *(appClient->QLServer()->Query(Ƒ( "purgeSession(id:\"{:x}\")", _request.SessionId() ), {}, appClient->UserPK()) );
			Resume( move(_request) );
		}
		catch( IException& e )
		{}
	}
	α HttpRequestAwait::Query()ι->TAwait<HttpTaskResult>::Task{
		try{
			string query = _request.IsGet() ? _request["query"] : Json::AsString(_request.Body(), "query" );
			THROW_IFX( query.empty(), RestException<http::status::bad_request>(SRCE_CUR, move(_request), "no query") );
			string variableString = _request.IsGet() ? _request["variables"] : Json::FindString(_request.Body(), "variables" ).value_or( "" );
			jobject variables = variableString.size() ? Json::AsObject( parse(move(variableString)) ) : jobject{};
			_request.LogRead( query );
			auto ql = QL::Parse( move(query), move(variables), Schemas(), _request.Params().contains("raw") );
			THROW_IFX( ql.IsMutation() && !_request.IsPost(), RestException<http::status::bad_request>(SRCE_CUR, move(_request), "Mutations must use post.") );
			Resume( co_await GatewayQLAwait{move(_request), move(ql)} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α HttpRequestAwait::Suspend()ι->void{
 		if( _request.IsPost("/login") ) //used with user/password on Opc Server.
			Login( _request.UserEndpoint.address().to_string() );
		else if( _request.IsPost("/logout") )
			Logout();
		else if( _request.IsGet("/graphql") || _request.IsPost("/graphql") )
			Query();
		else{
			auto opc = _request["opc"];
			if( opc.size() )
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
			auto rest = dynamic_cast<IRestException*>( e.get() );
			if( rest )
				rest->Throw();
			else{
				auto ua = dynamic_cast<UAClientException*>( e.get() );
				if( ua )
					ua->ThrowRest( move(*ua), move(_request) );
				else
					throw RestException<>{ move(*e), move(_request) };
			}
		}
		return _readyResult
			? HttpTaskResult{ move(*_readyResult), move(_request) }
			: Promise()->Value() ? move( *Promise()->Value() ) : HttpTaskResult{ jobject{}, move(_request) };
	}
}