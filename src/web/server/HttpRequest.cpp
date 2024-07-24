#include <jde/web/server/HttpRequest.h>
#include <jde/web/server/Flex.h>
//#include <jde/web/client/IClientSocketSession.h>//maxloglength
#define var const auto

namespace Jde{
	static sp<LogTag> _httpServerReadTag = Logging::Tag( ELogTags::HttpServerRead );
	static sp<LogTag> _httpServerWriteTag = Logging::Tag( ELogTags::HttpServerWrite );
	α Web::HttpServerReadTag()ι->sp<LogTag>{ return _httpServerReadTag; }
	α Web::HttpServerWriteTag()ι->sp<LogTag>{ return _httpServerWriteTag; }
}
namespace Jde::Web::Server{
	static atomic<uint32> _sequence = 0;
	string _accessControlAllowOrigin = Settings::Get("http/accessControl/allowOrigin").value_or("*");
	α AccessControlAllowOrigin()ι->string{ return _accessControlAllowOrigin; };

	HttpRequest::HttpRequest( TRequestType&& request, tcp::endpoint userEndpoint, uint32 connectionId )ι:
		UserEndpoint{ move(userEndpoint) },
		_request{ move(request) },
		_start{ steady_clock::now() },
		_connectionId{ connectionId },
		_index{ ++_sequence }{
		ParseUri();
	}

	α HttpRequest::ParseUri()->void{
		var& uri = Ssl::DecodeUri( _request.target() );
	  _target = uri.substr( 0, uri.find('?') );
		if( _target.size()+1<uri.size() ){
			var start = _target.size()+1;
			sv paramString = sv{ uri.data()+start, uri.size()-start };
			var paramStringSplit = Str::Split( paramString, '&' );
			for( auto param : paramStringSplit ){
				var keyValue = Str::Split( param, '=' );
				_params[string{keyValue[0]}]=keyValue.size()==2 ? string{keyValue[1]} : string{};
			}
		}
	}

	α HttpRequest::Response( json j, SL sl )Ι->http::response<http::string_body>{
		auto y = Response<http::string_body>();
		y.body() = j.dump();
		y.prepare_payload();
		Trace( sl, ELogTags::HttpServerWrite, "[{:x}.{:x}.{:x}]HttpResponse:  {}{} - {}", SessionInfo->SessionId, _connectionId, _index, Target(), y.body().substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(_start-steady_clock::now()) );
		return y;
	}

	α HttpRequest::LogRead( str text, SL sl )Ι->void{
		Trace( sl, ELogTags::HttpServerRead, "[{:x}.{:x}.{:x}]HttpRequest:  {}{} - {}", SessionInfo->SessionId, _connectionId, _index, Target(), text.substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(_start-steady_clock::now()) );
	}
}