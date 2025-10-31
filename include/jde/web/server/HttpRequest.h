#pragma once
#include <jde/db/usings.h>
#include <jde/fwk/io/json.h>
#include "Sessions.h"
#include "usings.h"

namespace Jde::Web::Server{
	α AccessControlAllowOrigin()ι->string;
	α ServerVersion( bool isSsl )ι->string;

	struct IRestException;
	struct ΓWS HttpRequest final{
		HttpRequest( TRequestType&& request, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionId )ι;
		HttpRequest( const HttpRequest& ) = delete;
		HttpRequest( HttpRequest&& ) = default;
		α operator=( const HttpRequest& ) = delete;
		α operator[]( str x )Ι->const string&;

		α StringBody()Ι->const string&{ return _request.body(); }
		α Body()ε->jobject&;
		α Body()Ε->const jobject&{ return const_cast<HttpRequest*>(this)->Body(); }
		α Contains( str param )Ι->bool{ return _params.contains( param ); }
		α Header( sv header )Ι->string{ return _request.base()[header]; }
		α IsGet()Ι->bool{ return _request.method() == http::verb::get; }
		α IsGet( str target)Ι->bool{ return _request.method() == http::verb::get && Target()==target; }
		α IsPost()Ι->bool{ return _request.method() == http::verb::post; }
		α IsPost( str target)Ι->bool{ return _request.method() == http::verb::post && Target()==target; }
		α KeepAlive()Ι->bool{ return _request.keep_alive(); }//TODO get rid of
		α Method()Ι->http::verb{ return _request.method(); }
		α Params()Ι->const flat_map<string,string>&{ return _params; }
		α SessionId()Ι->SessionPK;
		α Target()Ι->const string&{ return _target; }
		α UserPK()Ι->UserPK{ return SessionInfo->UserPK; }
		α Version()Ι->uint{ return _request.version(); }//TODO get rid of

		ψ BadRequest( SL sl, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>;
		α LogRead( str text="", SRCE )Ι->void;
		template<class T=http::string_body>
		α Response( http::status status=http::status::ok )Ι->http::response<T>;
		α Response( jvalue j, SRCE )Ι->http::response<http::string_body>;
		sp<Server::SessionInfo> SessionInfo;
		const tcp::endpoint UserEndpoint;
		flat_map<string,string> ResponseHeaders;

	private:
		α ParseUri()->void;

		mutable optional<jobject> _body;
		uint32 _connectionId;
		uint32 _index;
		bool _isSsl;
		flat_map<string,string> _params;
		TRequestType _request;
		steady_clock::time_point _start;
		string _target;

		friend IRestException;
	};

	template<class T>
	α HttpRequest::Response( http::status status )Ι->http::response<T>{
		http::response<T> res{ status, _request.version() };
		res.set( http::field::server, ServerVersion(_isSsl) );
		res.set( http::field::access_control_allow_origin, AccessControlAllowOrigin() );
		if( SessionInfo && SessionInfo->IsInitialRequest ){
			res.set( http::field::access_control_expose_headers, "Authorization" );
			res.set( http::field::authorization, Jde::format("{:x}", SessionInfo->SessionId) );
		}
		for( auto& [key,value] : ResponseHeaders )
			res.set( key, value );
		res.keep_alive( _request.keep_alive() );
		return res;
	}

	template<class... Args>
	α HttpRequest::BadRequest( SL sl, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>{
		auto res = Response<http::string_body>( http::status::bad_request );
		res.body() = Jde::format( format, args... );
		res.prepare_payload();
		//LogHttpServerSent( *this, res ); try log at end
		return res;
	}
}