#include <jde/web/client/http/ClientHttpRes.h>
#include <zlib.h>
#include <jde/web/client/usings.h>
#define let const auto

namespace Jde::Web::Client{
	α GetBody( const http::response<http::string_body>& res )ε->string{
		string body{ res.body() };
		if( res.base()[http::field::content_encoding]=="gzip" ){
      z_stream zs{ (Bytef*)body.data(), (uInt)body.size() }; //zs.zalloc = Z_NULL; zs.zfree = Z_NULL; zs.opaque = Z_NULL;
      string unziped;
      auto hr = inflateInit2( &zs, MAX_WBITS | 16 ); CHECK(hr==Z_OK);
      hr=Z_BUF_ERROR;
      for( auto outputSize = body.size(); hr==Z_BUF_ERROR; outputSize*=2 ){
				unziped.resize( outputSize );
         zs.avail_out = (uInt)unziped.size(); zs.next_out = (Bytef*)unziped.data()+zs.total_out;
         hr = inflate( &zs, Z_FINISH );
      }
      CHECK( hr==Z_STREAM_END );
      hr = inflateEnd( &zs );
			unziped.resize( zs.total_out );
			body = move( unziped );
		}
   	return body;
	}
	ClientHttpRes::ClientHttpRes( const http::response<http::string_body>& res )ε:
		_body{ GetBody(res) },
		_status{ res.result() },
		_headers{ res.base() }
	{}

	α ClientHttpRes::RedirectVariables()Ι->tuple<string,string,PortType>{
		let location = _headers[http::field::location];
		let startHost = location.find_first_of( "//" );
		if( startHost==string::npos || startHost+3>location.size() )
			THROW( "Could not parse redirect:  {}", location );
		let startTarget = location.find_first_of( "/", startHost+2 );
		auto host = location.substr( startHost+2, startTarget-startHost-2 );
		auto target = startTarget==string::npos ? string{} : string{ location.substr(startTarget) };
		constexpr PortType defaultPort{ 443 };
		return make_tuple( move(host), move(target), defaultPort );
	}
}