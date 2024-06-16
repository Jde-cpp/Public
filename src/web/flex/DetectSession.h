#pragma once

namespace Jde::Web::Flex{

	struct DetectSession : public std::enable_shared_from_this<DetectSession>{
    explicit DetectSession( tcp::socket&& socket, ssl::context& ctx ): 
			_stream( move(socket) ), 
			_ctx( ctx )
    {}

    // Launch the detector
    α Run()->void{
        // We need to be executing within a strand to perform async operations on the I/O objects in this session. Although not strictly necessary for single-threaded contexts, this example code is written to be thread-safe by default.
      net::dispatch( _stream.get_executor(), beast::bind_front_handler( &DetectSession::OnRun, this->shared_from_this()) );
    }

    α OnRun()->void{
      _stream.expires_after( std::chrono::seconds(30) );
      beast::async_detect_ssl( _stream, _buffer, beast::bind_front_handler(&DetectSession::OnDetect, this->shared_from_this()) );
    }
    α OnDetect( beast::error_code ec, bool result )->void{
			if( ec )
        return fail( ec, "detect" );
      
			if(result)
        ms<ssl_http_session>( move(_stream), _ctx, move(_buffer) )->Run();
			else
        ms<plain_http_session>( move(_stream), move(_buffer) )->Run();
    }
	private:
    beast::tcp_stream _stream;
    ssl::context& _ctx;
    beast::flat_buffer _buffer;
	};
}