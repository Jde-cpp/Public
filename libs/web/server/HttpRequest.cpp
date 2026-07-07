#include <jde/web/server/HttpRequest.h>
#include <jde/web/server/Server.h>
#include <jde/fwk/str.h>
#include <jde/fwk/chrono.h>

#define let const auto

namespace Jde::Web{
	string _accessControlAllowOrigin;
	α Server::AccessControlAllowOrigin()ι->string{
		if( _accessControlAllowOrigin.empty() )
			_accessControlAllowOrigin = Settings::FindString( "http/accessControl/allowOrigin" ).value_or( "*" );
		return _accessControlAllowOrigin;
	}

	string _plainVersion{ Ƒ("({})Jde.Web.Server - {}", Process::ProductVersion, BOOST_BEAST_VERSION) };
	string _sslVersion{ Ƒ("({})Jde.Web.Server SSL - {}", Process::ProductVersion, BOOST_BEAST_VERSION) };
	α Server::ServerVersion( bool isSsl )ι->string{ return isSsl ? _sslVersion : _plainVersion; }//TODO cache
}

namespace Jde::Web::Server{
	static atomic<uint32> _sequence = 0;

	HttpRequest::HttpRequest( TRequestType&& request, tcp::endpoint userEndpoint, bool isSsl, uint32 connectionId )ι:
		UserEndpoint{ move(userEndpoint) },
		_connectionId{ connectionId },
		_index{ ++_sequence },
		_isSsl{ isSsl },
		_request{ move(request) },
		_start{ steady_clock::now() }{
		ParseUri();
	}

	α HttpRequest::operator[]( str x )Ι->const string&{
		auto p = _params.find( x );
		return p!=_params.end() ? p->second : Str::Empty();
	}

	α HttpRequest::Body()ε->jobject&{
		if( !_body ){
			if( auto& s = _request.body(); s.size() )
				_body = Json::Parse( move(s) );
			else
				_body = {};
		}
		return *_body;
	}
	α HttpRequest::ParseUri()->void{
		let uri = sv{ _request.target() };//split before decoding so encoded '&'/'=' in values don't corrupt parsing.
		let queryStart = uri.find( '?' );
		_target = Str::DecodeUri( uri.substr(0, queryStart) );
		if( queryStart!=sv::npos ){
			for( let& param : Str::Split(uri.substr(queryStart+1), '&') ){
				let keyValue = Str::Split( param, '=' );
				if( keyValue.empty() )
					continue;
				_params[Str::DecodeUri(keyValue[0])] = keyValue.size()==2 ? Str::DecodeUri(keyValue[1]) : string{};
			}
		}
	}

	α HttpRequest::Response( jvalue j, SL sl )Ι->http::response<http::string_body>{
		auto y = Response<http::string_body>();
		y.body() = serialize( move(j) );
		y.prepare_payload();
		LOGSL( ELogLevel::Debug, sl, ELogTags::HttpServerWrite, "[{}.{}.{}]HttpResponse:  {}{} - {}", hex(SessionInfo ? SessionInfo->SessionId : 0), hex(_connectionId), hex(_index), Target(), y.body().substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(steady_clock::now()-_start) );
		return y;
	}

	α HttpRequest::SessionId()Ι->SessionPK{
		SessionPK sessionId{ SessionInfo ? SessionInfo->SessionId : 0 };
		if( auto authorization = sessionId ? string{} : Header("authorization"); authorization.size() )
			sessionId = Str::TryTo<SessionPK>( authorization, nullptr, 16 ).value_or( 0 );

		return sessionId;
	}

	α HttpRequest::LogRead( str text, ELogLevel level, SL sl )Ι->void{
		LOGSL( level, sl, ELogTags::HttpServerRead, "[{:x}.{:x}.{:x}]HttpRequest:  {} - {} - {}", SessionInfo->SessionId, _connectionId, _index, Target(), text.substr(0, MaxLogLength()), Chrono::ToString<steady_clock::duration>(steady_clock::now()-_start) );
	}
}