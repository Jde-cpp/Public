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
      hr = Z_BUF_ERROR;
      for( auto nextOutSize = std::max<size_t>(body.size(), 512); hr==Z_BUF_ERROR && zs.avail_out==0; nextOutSize*=2 ){//Z_BUF_ERROR with unused output space means truncated input, not a full buffer - stop instead of growing forever.
				let start = unziped.size();
				unziped.resize( start+nextOutSize );
				zs.next_out = (Bytef*)unziped.data()+start;
        zs.avail_out = nextOutSize;
        hr = inflate( &zs, Z_FINISH );
      }
			let totalOut = zs.total_out;
			inflateEnd( &zs );
      CHECK( hr==Z_STREAM_END );
			unziped.resize( totalOut );
			body = move( unziped );
		}
   	return body;
	}
	ClientHttpRes::ClientHttpRes( const http::response<http::string_body>& res )ε:
		_body{ GetBody(res) },
		_status{ res.result() },
		_headers{ res.base() }
	{}

	α ClientHttpRes::RedirectVariables()Ε->tuple<string,string,PortType>{
		let location = string{ _headers[http::field::location] };
		string host, target;
		PortType port{ 443 };
		if( let schemeEnd = location.find("://"); schemeEnd!=string::npos ){
			if( location.starts_with("http:") )
				port = 80;
			let hostStart = schemeEnd+3;
			let targetStart = location.find( '/', hostStart );
			host = location.substr( hostStart, targetStart==string::npos ? string::npos : targetStart-hostStart );
			if( let colon = host.find(':'); colon!=string::npos ){
				let portText = host.substr( colon+1 );
				if( portText.empty() || portText.find_first_not_of("0123456789")!=string::npos )
					THROW( "Could not parse redirect:  {}", location );
				port = (PortType)std::stoul( portText );
				host.resize( colon );
			}
			if( host.empty() )
				THROW( "Could not parse redirect:  {}", location );
			target = targetStart==string::npos ? string{"/"} : location.substr( targetStart );
		}
		else if( location.starts_with('/') )
			target = location;//relative redirect - caller reuses the original host & port.
		else
			THROW( "Could not parse redirect:  {}", location );
		return make_tuple( move(host), move(target), port );
	}
}