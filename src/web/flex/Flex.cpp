#include <jde/web/flex/Flex.h>
#include "CancellationSignals.h"

namespace Jde::Web{
	namespace Flex{ α LoadServerCertificate( ssl::context& ctx )->void; }
	static sp<LogTag> _logTag = Logging::Tag( "web" );
	α Flex::WebTag()ι->sp<LogTag>{ return _logTag; }

	bool _started{};
	α Flex::HasStarted()ι->bool{ return _started; }

	α Flex::Start( PortType port, sv address, uint8 threadCount )ε->void{
		if( address.empty() )
			address = "0.0.0.0";
		ssl::context ctx{ ssl::context::tlsv12 };
		LoadServerCertificate( ctx );
		CancellationSignals cancellation;
    net::io_context ioc{ threadCount };
    net::co_spawn( ioc, Listen(ctx, tcp::endpoint{net::ip::make_address(address), port}, cancellation), net::bind_cancellation_slot(cancellation.slot(), net::detached) );

    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait( [&]( const beast::error_code&, int sig ){
     	if( sig == SIGINT )
				cancellation.emit(net::cancellation_type::all);
			else
				ioc.stop(); // Stop the `io_context`. This will cause `run()` to return immediately, eventually destroying the `io_context` and all of the sockets in it.
    });

    std::vector<std::jthread> v; // Run the I/O service on the requested number of threadCount
    v.reserve( threadCount - 1 );
    for( auto i = threadCount - 1; i > 0; --i ){
      v.emplace_back( [&ioc]{ ioc.run(); } );
		}
		_started = true;
    ioc.run();
    for( auto& t : v )// (If we get here, it means we got a SIGINT or SIGTERM)
      t.join();
	}

	α Flex::Listen( ssl::context& ctx, tcp::endpoint endpoint, CancellationSignals& sig )->net::awaitable<void, executor_type>{
    typename tcp::acceptor::rebind_executor<executor_with_default>::other acceptor{ co_await net::this_coro::executor };
    if( !InitListener(acceptor, endpoint) )
      co_return;

    while( (co_await net::this_coro::cancellation_state).cancelled() == net::cancellation_type::none ){
			auto [ec, sock] = co_await acceptor.async_accept();
			const auto exec = sock.get_executor();
			var userEndpoint = sock.remote_endpoint();
			if( !ec )
				net::co_spawn( exec, DetectSession(StreamType(move(sock)), ctx, move(userEndpoint)), net::bind_cancellation_slot(sig.slot(), net::detached) );// We dont't need a strand, since the awaitable is an implicit strand.
    }
	}

	α Flex::InitListener( typename tcp::acceptor::rebind_executor<executor_with_default>::other& acceptor, const tcp::endpoint& endpoint )->bool{
    beast::error_code ec;
    acceptor.open( endpoint.protocol(), ec );
    if( ec ){
			Fail( ec, "open" );
			return false;
    }
    
    acceptor.set_option( net::socket_base::reuse_address(true), ec );// Allow address reuse
    if( ec ){
			Fail( ec, "set_option" );
			return false;
    }
    
    acceptor.bind( endpoint, ec );// Bind to the server address
    if( ec ){
			Fail( ec, "bind" );
			return false;
    }
    acceptor.listen( net::socket_base::max_listen_connections, ec );
    if( ec ){
			Fail( ec, "listen" );
			return false;
    }
    return true;
	}

	α Flex::DetectSession( StreamType stream, net::ssl::context& ctx, tcp::endpoint userEndpoint)->net::awaitable<void, executor_type>{
    beast::flat_buffer buffer;
    stream.expires_after( std::chrono::seconds(30) );// Set the timeout.
    auto [ec, result] = co_await beast::async_detect_ssl( stream, buffer );// on_run
    if( ec )
			co_return Fail(ec, "detect");
    if( result ){
			beast::ssl_stream<StreamType> ssl_stream{ move(stream), ctx };
			auto [ec, bytes_used] = co_await ssl_stream.async_handshake( net::ssl::stream_base::server, buffer.data() );
			if(ec)
				co_return Fail( ec, "handshake" );

			buffer.consume(bytes_used);
			co_await RunSession( ssl_stream, buffer, move(userEndpoint), true );
    }
    else
			co_await RunSession( stream, buffer, move(userEndpoint), false );
	}	

	α Flex::LoadServerCertificate( ssl::context& ctx )->void{//TODO
 		const string cert =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDlTCCAn2gAwIBAgIUOLxr3q7Wd/pto1+2MsW4fdRheCIwDQYJKoZIhvcNAQEL\n"
        "BQAwWjELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMRQwEgYDVQQHDAtMb3MgQW5n\n"
        "ZWxlczEOMAwGA1UECgwFQmVhc3QxGDAWBgNVBAMMD3d3dy5leGFtcGxlLmNvbTAe\n"
        "Fw0yMTA3MDYwMTQ5MjVaFw00ODExMjEwMTQ5MjVaMFoxCzAJBgNVBAYTAlVTMQsw\n"
        "CQYDVQQIDAJDQTEUMBIGA1UEBwwLTG9zIEFuZ2VsZXMxDjAMBgNVBAoMBUJlYXN0\n"
        "MRgwFgYDVQQDDA93d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IB\n"
        "DwAwggEKAoIBAQCz0GwgnxSBhygxBdhTHGx5LDLIJSuIDJ6nMwZFvAjdhLnB/vOT\n"
        "Lppr5MKxqQHEpYdyDYGD1noBoz4TiIRj5JapChMgx58NLq5QyXkHV/ONT7yi8x05\n"
        "P41c2F9pBEnUwUxIUG1Cb6AN0cZWF/wSMOZ0w3DoBhnl1sdQfQiS25MTK6x4tATm\n"
        "Wm9SJc2lsjWptbyIN6hFXLYPXTwnYzCLvv1EK6Ft7tMPc/FcJpd/wYHgl8shDmY7\n"
        "rV+AiGTxUU35V0AzpJlmvct5aJV/5vSRRLwT9qLZSddE9zy/0rovC5GML6S7BUC4\n"
        "lIzJ8yxzOzSStBPxvdrOobSSNlRZIlE7gnyNAgMBAAGjUzBRMB0GA1UdDgQWBBR+\n"
        "dYtY9zmFSw9GYpEXC1iJKHC0/jAfBgNVHSMEGDAWgBR+dYtY9zmFSw9GYpEXC1iJ\n"
        "KHC0/jAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBzKrsiYywl\n"
        "RKeB2LbddgSf7ahiQMXCZpAjZeJikIoEmx+AmjQk1bam+M7WfpRAMnCKooU+Utp5\n"
        "TwtijjnJydkZHFR6UH6oCWm8RsUVxruao/B0UFRlD8q+ZxGd4fGTdLg/ztmA+9oC\n"
        "EmrcQNdz/KIxJj/fRB3j9GM4lkdaIju47V998Z619E/6pt7GWcAySm1faPB0X4fL\n"
        "FJ6iYR2r/kJLoppPqL0EE49uwyYQ1dKhXS2hk+IIfA9mBn8eAFb/0435A2fXutds\n"
        "qhvwIOmAObCzcoKkz3sChbk4ToUTqbC0TmFAXI5Upz1wnADzjpbJrpegCA3pmvhT\n"
        "7356drqnCGY9\n"
        "-----END CERTIFICATE-----\n";

    const string key =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCz0GwgnxSBhygx\n"
        "BdhTHGx5LDLIJSuIDJ6nMwZFvAjdhLnB/vOTLppr5MKxqQHEpYdyDYGD1noBoz4T\n"
        "iIRj5JapChMgx58NLq5QyXkHV/ONT7yi8x05P41c2F9pBEnUwUxIUG1Cb6AN0cZW\n"
        "F/wSMOZ0w3DoBhnl1sdQfQiS25MTK6x4tATmWm9SJc2lsjWptbyIN6hFXLYPXTwn\n"
        "YzCLvv1EK6Ft7tMPc/FcJpd/wYHgl8shDmY7rV+AiGTxUU35V0AzpJlmvct5aJV/\n"
        "5vSRRLwT9qLZSddE9zy/0rovC5GML6S7BUC4lIzJ8yxzOzSStBPxvdrOobSSNlRZ\n"
        "IlE7gnyNAgMBAAECggEAY0RorQmldGx9D7M+XYOPjsWLs1px0cXFwGA20kCgVEp1\n"
        "kleBeHt93JqJsTKwOzN2tswl9/ZrnIPWPUpcbBlB40ggjzQk5k4jBY50Nk2jsxuV\n"
        "9A9qzrP7AoqhAYTQjZe42SMtbkPZhEeOyvCqxBAi6csLhcv4eB4+In0kQo7dfvLs\n"
        "Xu/3WhSsuAWqdD9EGnhD3n+hVTtgiasRe9318/3R9DzP+IokoQGOtXm+1dsfP0mV\n"
        "8XGzQHBpUtJNn0yi6SC4kGEQuKkX33zORlSnZgT5VBLofNgra0THd7x3atOx1lbr\n"
        "V0QizvCdBa6j6FwhOQwW8UwgOCnUbWXl/Xn4OaofMQKBgQDdRXSMyys7qUMe4SYM\n"
        "Mdawj+rjv0Hg98/xORuXKEISh2snJGKEwV7L0vCn468n+sM19z62Axz+lvOUH8Qr\n"
        "hLkBNqJvtIP+b0ljRjem78K4a4qIqUlpejpRLw6a/+44L76pMJXrYg3zdBfwzfwu\n"
        "b9NXdwHzWoNuj4v36teGP6xOUwKBgQDQCT52XX96NseNC6HeK5BgWYYjjxmhksHi\n"
        "stjzPJKySWXZqJpHfXI8qpOd0Sd1FHB+q1s3hand9c+Rxs762OXlqA9Q4i+4qEYZ\n"
        "qhyRkTsl+2BhgzxmoqGd5gsVT7KV8XqtuHWLmetNEi+7+mGSFf2iNFnonKlvT1JX\n"
        "4OQZC7ntnwKBgH/ORFmmaFxXkfteFLnqd5UYK5ZMvGKTALrWP4d5q2BEc7HyJC2F\n"
        "+5lDR9nRezRedS7QlppPBgpPanXeO1LfoHSA+CYJYEwwP3Vl83Mq/Y/EHgp9rXeN\n"
        "L+4AfjEtLo2pljjnZVDGHETIg6OFdunjkXDtvmSvnUbZBwG11bMnSAEdAoGBAKFw\n"
        "qwJb6FNFM3JnNoQctnuuvYPWxwM1yjRMqkOIHCczAlD4oFEeLoqZrNhpuP8Ij4wd\n"
        "GjpqBbpzyVLNP043B6FC3C/edz4Lh+resjDczVPaUZ8aosLbLiREoxE0udfWf2dU\n"
        "oBNnrMwwcs6jrRga7Kr1iVgUSwBQRAxiP2CYUv7tAoGBAKdPdekPNP/rCnHkKIkj\n"
        "o13pr+LJ8t+15vVzZNHwPHUWiYXFhG8Ivx7rqLQSPGcuPhNss3bg1RJiZAUvF6fd\n"
        "e6QS4EZM9dhhlO2FmPQCJMrRVDXaV+9TcJZXCbclQnzzBus9pwZZyw4Anxo0vmir\n"
        "nOMOU6XI4lO9Xge/QDEN4Y2R\n"
        "-----END PRIVATE KEY-----\n";

    const string dh =
        "-----BEGIN DH PARAMETERS-----\n"
        "MIIBCAKCAQEArzQc5mpm0Fs8yahDeySj31JZlwEphUdZ9StM2D8+Fo7TMduGtSi+\n"
        "/HRWVwHcTFAgrxVdm+dl474mOUqqaz4MpzIb6+6OVfWHbQJmXPepZKyu4LgUPvY/\n"
        "4q3/iDMjIS0fLOu/bLuObwU5ccZmDgfhmz1GanRlTQOiYRty3FiOATWZBRh6uv4u\n"
        "tff4A9Bm3V9tLx9S6djq31w31Gl7OQhryodW28kc16t9TvO1BzcV3HjRPwpe701X\n"
        "oEEZdnZWANkkpR/m/pfgdmGPU66S2sXMHgsliViQWpDCYeehrvFRHEdR9NV+XJfC\n"
        "QMUk26jPTIVTLfXmmwU0u8vUkpR7LQKkwwIBAg==\n"
        "-----END DH PARAMETERS-----\n";		
		ctx.set_password_callback( [](uint, boost::asio::ssl::context_base::password_purpose){ return "test"; } );
    ctx.set_options( boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::single_dh_use );

    ctx.use_certificate_chain( boost::asio::buffer(cert.data(), cert.size()) );
    ctx.use_private_key( boost::asio::buffer(key.data(), key.size()), boost::asio::ssl::context::file_format::pem );
    ctx.use_tmp_dh( boost::asio::buffer(dh.data(), dh.size()) );
	}
}