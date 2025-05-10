#include <jde/web/server/HttpRequest.h>
#include <jde/web/server/Server.h>
#include <jde/framework/str.h>
#include "../../../../Framework/source/DateTime.h"

#define let const auto

namespace Jde::Web{
	string _accessControlAllowOrigin = Settings::FindString("http/accessControl/allowOrigin").value_or("*");
	α Server::AccessControlAllowOrigin()ι->string{ return _accessControlAllowOrigin; };

	string _plainVersion{ Ƒ("({})Jde.Web.Server - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION) };
	string _sslVersion{ Ƒ("({})Jde.Web.Server SSL - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION) };
	α Server::ServerVersion( bool isSsl )ι->string{ return isSsl ? _sslVersion : _plainVersion; }//TODO cache
}

namespace Jde::Web::Server{
	static atomic<uint32> _sequence = 0;

	HttpRequest::HttpRequest( TRequestType&& request, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionId )ι:
		UserEndpoint{ move(userEndpoint) },
		_connectionId{ connectionId },
		_index{ ++_sequence },
		_isSsl{ isSsl },
		_request{ move(request) },
		_start{ steady_clock::now() }{
		ParseUri();
	}

	α HttpRequest::ParseUri()->void{
		let& uri = Str::DecodeUri( _request.target() );
	  _target = uri.substr( 0, uri.find('?') );
		if( _target.size()+1<uri.size() ){
			let start = _target.size()+1;
			sv paramString = sv{ uri.data()+start, uri.size()-start };
			let paramStringSplit = Str::Split( paramString, '&' );
			for( auto param : paramStringSplit ){
				let keyValue = Str::Split( param, '=' );
				_params[string{keyValue[0]}]=keyValue.size()==2 ? string{keyValue[1]} : string{};
			}
		}
	}

	α HttpRequest::Response( jvalue j, SL sl )Ι->http::response<http::string_body>{
		auto y = Response<http::string_body>();
		//if( !j.empty() )
		y.body() = serialize( move(j) );
		y.prepare_payload();
		Trace{ sl, ELogTags::HttpServerWrite, "[{:x}.{:x}.{:x}]HttpResponse:  {}{} - {}", SessionInfo ? SessionInfo->SessionId : 0, _connectionId, _index, Target(), y.body().substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(_start-steady_clock::now()) };
		return y;
	}

	α HttpRequest::SessionId()Ι->SessionPK{
		SessionPK sessionId{ SessionInfo ? SessionInfo->SessionId : 0 };
		if( auto authorization = sessionId ? string{} : Header("authorization"); authorization.size() )
			sessionId = Str::TryTo<SessionPK>( authorization, nullptr, 16 ).value_or( 0 );

		return sessionId;
	}

	α HttpRequest::LogRead( str text, SL sl )Ι->void{
		Trace{ sl, ELogTags::HttpServerRead, "[{:x}.{:x}.{:x}]HttpRequest:  {}{} - {}", SessionInfo->SessionId, _connectionId, _index, Target(), text.substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(_start-steady_clock::now()) };
	}
}