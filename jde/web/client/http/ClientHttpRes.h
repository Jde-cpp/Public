#pragma once
#include "../usings.h"
#include <jde/io/Json.h>

namespace Jde::Web::Client{

	struct ClientHttpRes final{
		ClientHttpRes()ι=default;//for await_resume default
		ClientHttpRes( const http::response<http::string_body>& res )ε;

		α operator[]( http::field field )Ι->string{ return _headers[field]; }

		α Body()Ι->string{ return _body; }
		α Headers()Ι->const http::header<true, http::fields>&{ return _headers; }
		α Json()Ι->json{ return Json::Parse(_body); }
		α IsError()Ι->bool{ using enum http::status; return _status!=ok && _status!=no_content && _status!=found; }
		α IsRedirect()Ι->bool{ return _status==http::status::moved_permanently || _status==http::status::found || _status==http::status::see_other || _status==http::status::temporary_redirect || _status==http::status::permanent_redirect; }
		α RedirectVariables()Ι->tuple<string,string,PortType>;
		α Status()Ι->http::status{ return _status; }
	private:
		string _body;
		http::status _status;
		http::header<true, http::fields> _headers;
	};
}