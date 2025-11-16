#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/fwk/process/execution.h>
#include <jde/web/client/http/ClientHttpSession.h>
#include <jde/web/client/http/ClientHttpResException.h>

#define let const auto

namespace Jde::Web{
	concurrent_flat_map<string,vector<sp<Client::ClientHttpSession>>> _sessions;
	α Client::RemoveHttpSession( sp<ClientHttpSession> session )ι->void{
		TRACET( ELogTags::HttpClientSessions, "[{}]Remove session: {}:{} {}.", hex((uint)session.get()), session->Host,session->Port, session->IsSsl ? "SSL" : "HTTP" );
		_sessions.erase_if( ClientHttpSession::Key(session->Host,session->Port, session->IsSsl), [session]( auto& kv ){
			auto& sessions = kv.second;
			if( auto p = find( sessions, session ); p!=sessions.end() ){
				TRACET( ELogTags::HttpClientSessions, "[{}]Remove session: {}:{} {}.", hex((uint)session.get()), session->Host,session->Port, session->IsSsl ? "SSL" : "HTTP" );
				sessions.erase( p );
			}
			return sessions.empty();
		});
	}
	struct SessionShutdown final : IShutdown{
		SessionShutdown()ι{ Execution::AddShutdown(this); }
		α Shutdown( bool terminate )ι->void{
			while( _sessions.size() ){
				_sessions.visit_all( []( auto&& kv )mutable{
					for( auto p = kv.second.begin(); p!=kv.second.end(); ){
						bool running = (*p)->IsRunning();
						if( !running )
							(*p)->Close();
						p = running ? std::next(p) : kv.second.erase( p );
					}
				});
				std::this_thread::sleep_for( 100ms );
				if( terminate )
					_sessions.clear();
				else
					_sessions.erase_if( [](auto&& kv){ return kv.second.empty(); } );
			};
		}
	};
	SessionShutdown _sessionShutdown;
}
namespace Jde::Web::Client{
	ClientHttpAwait::ClientHttpAwait( string host, string target, PortType port, HttpAwaitArgs args, SL sl )ι:
		base{ sl },
		HttpAwaitArgs{ move(args) },
		_host{host}, _target{target}, _port{port},
		_ioContext{ Executor() }{
		if( Verb==http::verb::unknown )
			Verb=http::verb::get;
	}
	ClientHttpAwait::ClientHttpAwait( string host, string target, string body, PortType port, HttpAwaitArgs args, SL sl )ι:
		base{ sl },
		HttpAwaitArgs{ move(args) },
		_host{host}, _target{target}, _body{body}, _port{port},
		_ioContext{ Executor() }{
		if( Verb==http::verb::unknown )
			Verb=http::verb::post;
	}
	ClientHttpAwait::ClientHttpAwait( ClientHttpAwait&& from )ι:
		base{ from._sl },
		HttpAwaitArgs{ move(from) },
		_host{ move(from._host) }, _target{ move(from._target) }, _body{ move(from._body) }, _port{ from._port }, _ioContext{ from._ioContext }
	{}

	α ClientHttpAwait::Execute()ι->ClientHttpAwaitSingle::Task{
		auto firstAttempt = ClientHttpAwaitSingle{ move(*this), _sl };
		bool retry{};
		try{
			auto res = co_await firstAttempt;
			Resume( move(res) );
		}
		catch( ClientHttpException& e ){
			if( !e.SslStreamTruncated() )
				ResumeExp( move(e) );
			else
				retry = true;
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
		if( retry ){
			try{
				Resume( co_await ClientHttpAwaitSingle{ move(firstAttempt) } );
			}
			catch( IException& e ){
				ResumeExp( move(e) );
			}
		}
	}
	α ClientHttpAwait::await_resume()ε->ClientHttpRes{
		ClientHttpRes res = base::await_resume();
		if( res.IsError() ){
			for( auto& h : res.Headers() )
				TRACET( ELogTags::Test, "{}: {}", h.name_string(), h.value() );
			throw ClientHttpResException( move(res) );
		}
		return res;
	}

	α ClientHttpAwaitSingle::Execute()ι->void{
		sp<ClientHttpSession> session;

		_sessions.visit( ClientHttpSession::Key(_host,_port, IsSsl), [&session]( auto& kv ){
			auto& sessions = kv.second;
			if( auto p=find_if(sessions, [](auto&& s){ return !s->IsRunning(); }); p!=sessions.end() ){
				session = *p;
				session->SetIsRunning( true );
			}
		});
		if( !session ){
      net::any_io_executor strand = net::make_strand( *_ioContext );
			session = IsSsl ?  ms<ClientHttpSession>( _host, _port, strand, _sl ) : ms<ClientHttpSession>( _host, _port, strand, true, true, _sl );
			TRACET( ELogTags::HttpClientSessions, "[{}]New session: {}:{} {}.", hex((uint)session.get()), _host, _port, IsSsl ? "SSL" : "HTTP" );
			_sessions.emplace_or_visit( ClientHttpSession::Key(_host,_port, IsSsl), vector<sp<ClientHttpSession>>{session}, [session]( auto&& kv ){ kv.second.push_back(session);} );
		}
		session->Send( _target, _body, *this, _h );
	}
	α ClientHttpAwaitSingle::await_resume()ε->ClientHttpRes{
		if( !Promise() )
			throw Exception( "Executor down." );
		return base::await_resume();
	}
}