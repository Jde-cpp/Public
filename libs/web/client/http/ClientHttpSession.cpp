#include <jde/web/client/http/ClientHttpSession.h>
#include <jde/web/client/http/ClientHttpRes.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include <jde/framework/thread/execution.h>


namespace Jde::Web::Client{
	Duration _handshakeTimeout{ Settings::FindDuration("web/client/timeoutHandshake").value_or(std::chrono::seconds(30)) };
	Duration _timeout{ Settings::FindDuration("web/client/timeout").value_or(std::chrono::seconds(30)) };
	Duration _closeTimeout{ Settings::FindDuration("web/client/timeoutClose").value_or(std::chrono::seconds(30)) };
	ssl::context _ctx{ ssl::context::tlsv12_client };// The SSL context is required, and should hold certificates
	ELogTags _tags{ ELogTags::HttpClientWrite };
	static string _userAgent{ Ƒ("({})Jde.Web.Client - {}", IApplication::ProductVersion, BOOST_BEAST_VERSION) };

	ClientHttpSession::ClientHttpSession( str host, PortType port, net::any_io_executor strand, bool isPlain, bool log )ε:
		Host{ host }, Port{ port }, IsSsl{ false }, _log{log}, _resolver{ strand },
		_stream{ beast::tcp_stream{strand} }{
		ASSERT( isPlain );
		_isRunning.test_and_set();
	}

	ClientHttpSession::ClientHttpSession( str host, PortType port, net::any_io_executor strand )ε:
		Host{ host }, Port{ port }, IsSsl{ true }, _log{true}, _resolver{ strand },
		_stream{ beast::ssl_stream<beast::tcp_stream>{strand, _ctx} }{
		_stream.SetSslTlsExtHostName(Host);
		_isRunning.test_and_set();
	}

	struct ConnectAwait final : VoidAwait<>{
		using base=VoidAwait<>;
		ConnectAwait( tcp::resolver::results_type&& resolvedResults, sp<ClientHttpSession> session, SRCE )ι:
			base{ sl },
			_resolvedResults{move(resolvedResults)},
			_session{session}
		{}
		α Suspend()ι->void override{
			_session->Stream().expires_after( _handshakeTimeout );
			_session->Stream().async_connect( _resolvedResults, beast::bind_front_handler(&ConnectAwait::OnConnect, this) );
		}
		α OnConnect( beast::error_code ec, tcp::resolver::results_type::endpoint_type )->void{
			if( ec )
				ResumeExp( ClientHttpException{ec} );
			else
				Resume();
		}
		tcp::resolver::results_type _resolvedResults;
		sp<ClientHttpSession> _session;
	};

	struct HandshakeAwait final : VoidAwait<>{
		using base=VoidAwait<>;
		HandshakeAwait( sp<ClientHttpSession> session, SRCE )ι:base{ sl }, _session{session}{}
		α Suspend()ι->void override{
			_session->Stream().expires_after( _handshakeTimeout );
			_session->Stream().async_handshake( beast::bind_front_handler(&HandshakeAwait::OnHandshake, this) );
		}
		α OnHandshake( beast::error_code ec )->void{
			if( ec )
				ResumeExp( ClientHttpException{ec} );
			else
				Resume();
		}
		sp<ClientHttpSession> _session;
	};

	struct MakeConnectionAwait final : VoidAwait<>{
		using base=VoidAwait<>;
		MakeConnectionAwait( sp<ClientHttpSession> session, SRCE )ι:base{ sl }, _session{session}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->ConnectAwait::Task{
			try{
				beast::error_code ec;
				auto resolvedResults = _session->Resolver().resolve( _session->Host, std::to_string(_session->Port), ec );//async resolve starts another thread.
				if( ec )
					throw ClientHttpException{ ec };
				co_await ConnectAwait{ move(resolvedResults), _session };
				if( _session->IsSsl )
					co_await HandshakeAwait{ _session };
				Resume();
			}
			catch( IException& e ){
				ResumeExp( move(e) );
			}
		}
		sp<ClientHttpSession> _session;
	};

	struct AsyncWriteAwait final : TAwait<ClientHttpRes>{
		using base=TAwait<ClientHttpRes>;
		AsyncWriteAwait( http::request<http::string_body> req, const HttpAwaitArgs& args, sp<ClientHttpSession> session, SRCE )ι:base{ sl }, _req{move(req)}, _args{args}, _session{session}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->void{
			_session->Stream().expires_after( _timeout );
			_session->Stream().async_write( _req, beast::bind_front_handler(&AsyncWriteAwait::OnWrite, this) );
		}
		α OnWrite( beast::error_code ec, std::size_t bytes_transferred )->void{
      boost::ignore_unused( bytes_transferred );
			if( ec )
				ResumeExp( ClientHttpException{ec, _session, &_req} );
			else
				_session->Stream().async_read( _res, _buffer, beast::bind_front_handler(&AsyncWriteAwait::OnRead, this) );
		}
		α OnRead( beast::error_code ec, std::size_t bytes_transferred )ι->ClientHttpAwait::Task{
      boost::ignore_unused( bytes_transferred );
			if( ec )
				ResumeExp( ClientHttpException{ec, _session, &_req} );
			else{
				ClientHttpRes res{ move(_res) };
				if( res.IsRedirect() && _session->AllowRedirects ){
					auto [host,target,port] = res.RedirectVariables();
					Debug( _tags, "redirecting from {}{} to {}", _session->Host, _req.target(), res[http::field::location] );
					try{
						res = co_await ClientHttpAwait{ host, target, _req.body(), port, _args, _sl };
					}
					catch( IException& ){
						ResumeExp( ClientHttpException{ec, _session, &_req} );
					}
				}
				Resume( move(res) );
			}
		}
	private:
		http::request<http::string_body> _req;
		const HttpAwaitArgs& _args;
		sp<ClientHttpSession> _session;
		beast::flat_buffer _buffer;
		http::response<http::string_body> _res;
	};

	struct ShutdownAwait final : VoidAwait<>{
		using base=VoidAwait<>;
		ShutdownAwait( sp<ClientHttpSession> session, SRCE )ι:base{ sl }, _session{session}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->void{
			_session->Stream().expires_after( _handshakeTimeout );
      _session->Stream().async_shutdown( [this](beast::error_code ec){ ShutdownAwait::OnShutdown(ec); } );
		}
		α OnShutdown( beast::error_code ec )->void{
			if( ec && (!_session->IsSsl || ( _session->IsSsl && ec !=net::error::eof)) )
				Trace( ELogTags::Shutdown | ELogTags::Http | ELogTags::Client, "shutdown: {}", ec.message() );
			Resume();
		}
	private:
		sp<ClientHttpSession> _session;
	};

  α ClientHttpSession::Send( string target, string body, const HttpAwaitArgs& args, ClientHttpAwaitSingle::Handle h )ι->VoidTask{
		try{
			if( !_isConnected ){
				co_await MakeConnectionAwait{ shared_from_this() };
				_isConnected = true;
			}
			Write( move(target), move(body), args, h );
		}
		catch( IException& e ){
			SetIsRunning( false );
			h.promise().ResumeExp( move(e), h );
		}
  }

	α ClientHttpSession::Write( string target, string body, const HttpAwaitArgs& args, ClientHttpAwaitSingle::Handle h )ι->AsyncWriteAwait::Task{
		ASSERT( args.Verb.has_value() );
		constexpr int version{ 11 };
		if( _log )
			Trace{ _tags, "{}:{}{} - {}", Host, Port, target, body.substr(0, Client::MaxLogLength()) };
		http::request<http::string_body> req{ *args.Verb, target, version };
		req.set( http::field::user_agent, _userAgent );
		req.set( http::field::content_type, args.ContentType );
		req.set( http::field::accept_encoding, "gzip" );
		if( args.Authorization.size() )
			req.set( http::field::authorization, args.Authorization );

		req.body() = move( body );
		req.prepare_payload();
		try{
			auto res = co_await AsyncWriteAwait{ move(req), args, shared_from_this() };
			if( _log )
				Trace{ ELogTags::HttpClientRead, "{}:{}{} - {}", Host, Port, target, res.Body().substr(0, Client::MaxLogLength()) };
			h.promise().SetValue( move(res) );
			//SetIsRunning( false );  //TODO implement keep-alive
		}
		catch( IException& e ){
			h.promise().SetExp( move(e) );
		}
		Close();
		h.resume();
	}

	α ClientHttpSession::Close()ε->VoidTask{
		co_await ShutdownAwait{ shared_from_this() };
	}
}