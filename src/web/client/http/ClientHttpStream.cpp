#include <jde/web/client/http/ClientHttpStream.h>

namespace Jde::Web::Client{

	α ClientHttpStream::SetSslTlsExtHostName(str host)ε->void{
		if( !SSL_set_tlsext_host_name( get<1>(_stream).native_handle(), host.c_str()) ){
			beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
			throw ClientHttpException{ ec, host };
		}
	}

	α ClientHttpStream::async_connect( const tcp::resolver::results_type& resolved, function<void(beast::error_code ec, tcp::resolver::results_type::endpoint_type)>&& token )ε->void{
		std::visit(
			[&resolved,&token](auto&& arg)->auto {
				return beast::get_lowest_layer(arg).async_connect( resolved, token );
			}, _stream );
	}

	α ClientHttpStream::async_write( const http::request<http::string_body>& req, function<void(beast::error_code, std::size_t)>&& token )ε->void{
		visit( [&req, &token](auto&& arg)->void {
			net::post( beast::get_lowest_layer(arg).get_executor(), [pReq=&req, token, pArg=&arg](){
				http::async_write( *pArg, *pReq, token );
			});
		}, _stream );
	}

	α ClientHttpStream::async_read( http::response<http::string_body>& res, beast::flat_buffer& buffer, function<void(beast::error_code, std::size_t)>&& token )ε->void{
		std::visit( [&buffer, &res, &token](auto&& arg)->void {
			http::async_read( arg, buffer, res, token );
		}, _stream );
	}
}