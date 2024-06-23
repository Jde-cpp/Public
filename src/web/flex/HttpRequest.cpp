#include <jde/web/flex/HttpRequest.h>
#define var const auto

namespace Jde::Web::Flex{

	string _accessControlAllowOrigin = Settings::Get("http/accessControl/allowOrigin").value_or("*");
	α AccessControlAllowOrigin()ι->string{ return _accessControlAllowOrigin; };

	HttpRequest::HttpRequest( TRequestType&& request, tcp::endpoint userEndpoint )ι:
		UserEndpoint{ move(userEndpoint) },
		_request{ move(request) },
		_start{ steady_clock::now() }{
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
		return y;
	}
}