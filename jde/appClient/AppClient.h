#pragma once
#include <jde/appClient/Sessions.h>

namespace Jde::App::Client{
	α SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void;
	α UpdateStatus()ι->void;
	α IsAppServer()ι->bool;

	struct SessionInfoAwait : TAwait<SessionInfo>{
		using base = TAwait<SessionInfo>;
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:base{sl}, _sessionId{sessionId}{}
		α await_ready()ι->bool{ return IApplication::ShuttingDown(); }
		α await_suspend( base::Handle h )ι->void;
		α await_resume()ε->SessionInfo;
		SessionPK _sessionId;
	};

	struct GraphQLAwait : TAwait<json>{
		using base = TAwait<json>;
		GraphQLAwait( string&& query, SRCE )ι:base{sl}, _query{move(query)}{}
		string _query;
	};

	//TODO change to functions, not returning anything.
	struct LogAwait : VoidAwait<>{
		using base = VoidAwait<>;
		LogAwait( Logging::ExternalMessage&& m, SRCE )ι:base{sl}, _message{move(m)}{}
		LogAwait( const Logging::ExternalMessage& m, const vector<string>* args, SRCE )ι:base{sl}, _message{ m }, _args{ args }{}
		const Logging::ExternalMessage _message;
		const vector<string>* _args;
	};



}