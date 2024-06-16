#pragma once
#include "HttpRequestAwait.h"
#include <jde/db/usings.h>
#include <jde/io/Json.h>
#include <jde/web/usings.h>
#include "Sessions.h"
#include "../../../../Framework/source/db/GraphQL.h"

namespace Jde::Web::Flex{
	using TBody=http::string_body;
	using TAllocator=std::allocator<char>;
	α WebTag()ι->sp<LogTag>;

	constexpr string ServerVersion()ι{ return Jde::format("({})Jde.Web - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION); }//TODO cache
	//template<class TBody, class TAllocator>
	struct HttpRequest final{
		using RequestType = http::request<TBody, http::basic_fields<TAllocator>>;
		HttpRequest( RequestType&& request, tcp::endpoint userEndpoint )ι;
		HttpRequest( const HttpRequest& ) = delete;
		HttpRequest( HttpRequest&& ) = default;
		α operator=( const HttpRequest& ) = delete;
		α operator[]( sv header )Ι->string{ return _request.base()[header]; }
		//α RemoteEndpoint()Ι->boost::asio::ip::tcp::endpoint{ return beast::get_lowest_layer(stream).remote_endpoint(); }

		α StringBody()Ι->const string&{ return _request.body(); }
		α Body()Ε->json{ return Json::Parse( _request.body() ); }
		α KeepAlive()Ι->bool{ return _request.keep_alive(); }//TODO get rid of
		α Method()Ι->http::verb{ return _request.method(); }
		α Params()Ι->const flat_map<string,string>&{ return _params; }
		α operator[]( str x )Ι->const string&{ auto p=_params.find(x); return p==_params.end() ? Str::Empty() : p->second; }
		α Target()Ι->const string&{ return _target; }
		α UserPK()Ι->UserPK{ return SessionInfo.UserPK; }
		α Version()Ι->uint{ return _request.version(); }//TODO get rid of
		ψ BadRequest( SL sl, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>;
		//ψ InternalServerError( IException&& e, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>;

		template<class T=http::string_body> α Response( http::status status=http::status::ok )Ι->http::response<T>;
		Sessions::Info SessionInfo;
		const tcp::endpoint UserEndpoint;
	private:
		α ParseUri()->void;

		flat_map<string,string> _params;
		RequestType _request;
		steady_clock::time_point _start;
		string _target;
		//HttpCo _coroutine{};
		//sp<IRestSession> Session;
		
	};

#define var const auto
//template<class TBody, class TAllocator>HttpRequest<TBody,TAllocator>
#define Class HttpRequest
#define ClassFunc α HttpRequest
	
	inline Class::HttpRequest( RequestType&& request, tcp::endpoint userEndpoint )ι:
		UserEndpoint{ move(userEndpoint) },
		_request{ move(request) },
		_start{ steady_clock::now() }{
		ParseUri(); 
	}

	inline ClassFunc::ParseUri()->void{
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
	#define _logTag WebTag()
	template<class T>
	ClassFunc::Response( http::status status )Ι->http::response<T>{
		http::response<T> res{ status, _request.version() };
		res.set( http::field::server, ServerVersion() );
		//res.set(http::field::content_type, "text/html");
		res.keep_alive( _request.keep_alive() );
		//res.body() = "The resource '" + std::string(target) + "' was not found.";
		//res.prepare_payload();//This function will adjust the Content-Length and Transfer-Encoding field values based on the properties of the body.
		return res;
	}
	template<class... Args>
	ClassFunc::BadRequest( SL sl, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>{ 
		auto res = Response<http::string_body>( http::status::bad_request ); 
		res.body() = Jde::format( format, args... );
		res.prepare_payload();
		TRACESL( "BadRequest={}", res.body() );
		return res;
	}	
/*	template<class... Args>
	ClassFunc::InternalServerError( IException&& e, fmt::format_string<Args...> format, Args&&... args )Ι->http::response<http::string_body>{ 
		auto res = Response<http::string_body>( http::status::internal_server_error ); 
		res.body() = Jde::format( format, args... );
		res.prepare_payload();
		auto& sl = e.Stack().front();
		TRACESL( "InternalServerError={}", res.body() );
		return res;
	}	*/
}
#undef var
#undef _logTag
#undef Class
#undef ClassFunc
