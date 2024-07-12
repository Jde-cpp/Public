#pragma once
//#include "HttpRequestAwait.h"
#include <jde/db/usings.h>
#include <jde/io/Json.h>
#include <jde/web/usings.h>
#include <jde/appClient/Sessions.h>
//#include "../../../../Framework/source/db/GraphQL.h"

namespace Jde::Web{
	namespace Flex{ struct HttpRequest; }
//	α LogHttpServerReceived( const Flex::HttpRequest& _request, const json& j )ι;
//	α LogHttpServerSent( const Flex::HttpRequest& _request, const json& j, ELogLevel l=ELogLevel::Trace )ι;
	α HttpServerSentTag()ι->sp<LogTag>;
	α HttpServerReceivedTag()ι->sp<LogTag>;
}

namespace Jde::Web::Flex{
	α WebTag()ι->sp<Jde::LogTag>;
	α AccessControlAllowOrigin()ι->string;

	constexpr string ServerVersion()ι{ return Jde::format("({})Jde.Web - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION); }//TODO cache
	//template<class TBody, class TAllocator>
	struct HttpRequest final{
		HttpRequest( TRequestType&& request, tcp::endpoint userEndpoint, uint32 connectionId )ι;
		HttpRequest( const HttpRequest& ) = delete;
		HttpRequest( HttpRequest&& ) = default;
		α operator=( const HttpRequest& ) = delete;
		α operator[]( str x )ι->string&{ return _params[x]; }
		//α RemoteEndpoint()Ι->boost::asio::ip::tcp::endpoint{ return beast::get_lowest_layer(stream).remote_endpoint(); }

		α StringBody()Ι->const string&{ return _request.body(); }
		α Body()Ε->json{ return Json::Parse( _request.body() ); }
		α Header( sv header )Ι->string{ return _request.base()[header]; }
		α KeepAlive()Ι->bool{ return _request.keep_alive(); }//TODO get rid of
		α Method()Ι->http::verb{ return _request.method(); }
		α Params()Ι->const flat_map<string,string>&{ return _params; }
		α Target()Ι->const string&{ return _target; }
		α UserPK()Ι->UserPK{ return SessionInfo.UserPK; }
		α Version()Ι->uint{ return _request.version(); }//TODO get rid of
		ψ BadRequest( SL sl, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>;
		//ψ InternalServerError( IException&& e, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>;
		α LogReceived( str text="" )Ι->void;

		template<class T=http::string_body> α Response( http::status status=http::status::ok )Ι->http::response<T>;
		α Response( json j )Ι->http::response<http::string_body>;
		Web::SessionInfo SessionInfo;
		const tcp::endpoint UserEndpoint;
		flat_map<string,string> ResponseHeaders;

	private:
		α ParseUri()->void;

		flat_map<string,string> _params;
		TRequestType _request;
		steady_clock::time_point _start;
		string _target;
		uint32 _connectionId;
		uint32 _index;
	};

	template<class T>
	α HttpRequest::Response( http::status status )Ι->http::response<T>{
		http::response<T> res{ status, _request.version() };
		res.set( http::field::server, ServerVersion() );
		res.set( http::field::access_control_allow_origin, AccessControlAllowOrigin() );
		if( SessionInfo.IsInitialRequest )
			res.set( http::field::authorization, Jde::format("{:x}", SessionInfo.SessionId) );
		for( auto& [key,value] : ResponseHeaders )
			res.set( key, value );
		//res.set(http::field::content_type, "text/html");
		res.keep_alive( _request.keep_alive() );
		//res.body() = "The resource '" + std::string(target) + "' was not found.";
		//res.prepare_payload();//This function will adjust the Content-Length and Transfer-Encoding field values based on the properties of the body.
		return res;
	}

	template<class... Args>
	α HttpRequest::BadRequest( SL sl, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>{
		auto res = Response<http::string_body>( http::status::bad_request );
		res.body() = Jde::format( format, args... );
		res.prepare_payload();
		//LogHttpServerSent( *this, res ); TODO try log at end
		return res;
	}
/*	template<class... Args>
	HttpRequest::InternalServerError( IException&& e, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>{
		auto res = Response<http::string_body>( http::status::internal_server_error );
		res.body() = Jde::format( format, args... );
		res.prepare_payload();
		auto& sl = e.Stack().front();
		TRACESL( "InternalServerError={}", res.body() );
		return res;
	}	*/

}
