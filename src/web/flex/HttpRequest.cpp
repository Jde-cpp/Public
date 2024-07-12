#include <jde/web/flex/HttpRequest.h>
#include <jde/web/flex/Flex.h>
#define var const auto

namespace Jde{
	static sp<LogTag> _httpServerSentTag = Logging::Tag( ELogTags::HttpServerSent );
	static sp<LogTag> _httpServerReceivedTag = Logging::Tag( ELogTags::HttpServerReceived );
	α Web::HttpServerSentTag()ι->sp<LogTag>{ return _httpServerSentTag; }
	α Web::HttpServerReceivedTag()ι->sp<LogTag>{ return _httpServerReceivedTag; }
}
namespace Jde::Web::Flex{
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
		var& uri = _request.target();
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

	α HttpRequest::Response( json j )Ι->http::response<http::string_body>{
		auto y = Response<http::string_body>();
		y.body() = j.dump();
		y.prepare_payload();
		TRACET( HttpServerSentTag(), "[{:x}.{:x}.{:x}]HttpRequest:  {}{} - {}", SessionInfo.SessionId, _connectionId, _index, Target(), y.body().substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(_start-steady_clock::now()) );
		return y;
	}

	α HttpRequest::LogReceived( str text )Ι->void{
		TRACET( HttpServerReceivedTag(), "[{:x}.{:x}.{:x}]HttpRequest:  {}{} - {}", SessionInfo.SessionId, _connectionId, _index, Target(), text.substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(_start-steady_clock::now()) );
	}
}