#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/thread/Execution.h>
#include <jde/web/client/http/ClientHttpSession.h>
#include <jde/web/client/http/ClientHttpResException.h>

#define var const auto

namespace Jde::Web{
	concurrent_flat_map<string,vector<sp<Client::ClientHttpSession>>> _sessions;
	α Client::RemoveHttpSession( sp<ClientHttpSession> pSession )ι{
		_sessions.erase_if( ClientHttpSession::Key(pSession->Host,pSession->Port, pSession->IsSsl), [pSession]( auto& kv ){
			auto& sessions = kv.second;
			if( auto p = find( sessions, pSession ); p!=sessions.end() )
				sessions.erase( p );
			return sessions.empty();
		});
	}
	struct SessionShutdown final : IShutdown{
		SessionShutdown()ι{ Execution::AddShutdown(this); }
		α Shutdown( bool /*terminate*/ )ι->void{
			while( _sessions.size() ){
				_sessions.visit_all( []( auto&& kv )mutable{
					for( auto p = kv.second.begin(); p!=kv.second.end(); ){
						bool running = (*p)->IsRunning();
						if( !running )
							(*p)->Close();
						p = running ? std::next(p) : kv.second.erase( p );
					}
				});
				_sessions.erase_if( [](auto&& kv){ return kv.second.empty(); } );
				_sessions.clear();
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
		auto firstAttempt = ClientHttpAwaitSingle{ move(*this) };
		try{
			auto res = co_await firstAttempt;
			SetValue( move(res) );
		}
		catch( ClientHttpException& e ){
			if( !e.SslStreamTruncated() )
				SetError( move(e) );
		}
		catch( IException& e ){
			SetError( move(e) );
		}
		if( !Emplaced() ){
			try{
				SetValue( co_await ClientHttpAwaitSingle{ move(firstAttempt) } );
			}
			catch( IException& e ){
				SetError( move(e) );
			}
		}
		Resume();
	}
	α ClientHttpAwait::await_resume()ε->ClientHttpRes{
		ClientHttpRes res = base::await_resume();
		if( res.IsError() )
			throw ClientHttpResException( move(res) );
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
			session = IsSsl ?  ms<ClientHttpSession>( _host, _port, strand ) : ms<ClientHttpSession>( _host, _port, strand, true );
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