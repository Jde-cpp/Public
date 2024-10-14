#pragma once
#include "ClientHttpException.h"

namespace Jde::Web::Client{
	struct ClientHttpStream final{
		ClientHttpStream( beast::tcp_stream&& stream )ι:_stream{std::move(stream)}{ Trace( ELogTags::HttpClientRead, "ClientHttpStream::ClientHttpStream" ); }
		ClientHttpStream( beast::ssl_stream<beast::tcp_stream>&& stream )ι:_stream{std::move(stream)}{}
		~ClientHttpStream(){}

		α SetSslTlsExtHostName(str host)ε->void;
		α expires_after( Duration d )ι->auto{ return std::visit( [d](auto&& arg)->auto { return beast::get_lowest_layer(arg).expires_after(d); }, _stream ); }
		α async_connect( const tcp::resolver::results_type& resolved, function<void(beast::error_code ec, tcp::resolver::results_type::endpoint_type)>&& token )ε->void;
		α async_handshake( function<void(beast::error_code ec)>&& token )ε->void{ get<1>( _stream ).async_handshake( ssl::stream_base::client, token ); }
		α async_write( const http::request<http::string_body>& req, function<void(beast::error_code, std::size_t)>&& token )ε->void;
		α async_read( http::response<http::string_body>& res, beast::flat_buffer& buffer, function<void(beast::error_code, std::size_t)>&& token )ε->void;

		α async_shutdown( function<void(beast::error_code ec)>&& token )ε->void{
			if( _stream.index()==0 ){
				ErrorCodeException ec;
				get<0>(_stream).socket().shutdown( tcp::socket::shutdown_both, ec );
				if( ec == beast::errc::not_connected )
					ec = {};
				token( ec );
			}
			else
				get<1>(_stream).async_shutdown( token );
		}
	private:
		std::variant<beast::tcp_stream,beast::ssl_stream<beast::tcp_stream>> _stream;
	};
}