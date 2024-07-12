#pragma once
#include <jde/appClient/Sessions.h>
#include <jde/appClient/usings.h>

namespace Jde::App{
	α IsAppServer()ι->bool; α SetIsAppServer( bool isAppServer )ι->void;
	// α AppId()ι->AppPK; α SetAppId( AppPK x )ι->void;
	// α InstanceId()ι->AppInstancePK; α SetInstanceId( AppInstancePK x )ι->void;

namespace Client{
	α UpdateStatus()ι->void;
	α SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void;

	struct SessionInfoAwait : TAwait<Web::SessionInfo>{
		using base = TAwait<Web::SessionInfo>;
		SessionInfoAwait( SessionPK sessionId, SRCE )ι:base{sl}, _sessionId{sessionId}{}
		α await_ready()ι->bool{ return IApplication::ShuttingDown(); }
		α await_suspend( base::Handle h )ι->void;
		α await_resume()ε->Web::SessionInfo;
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
}}