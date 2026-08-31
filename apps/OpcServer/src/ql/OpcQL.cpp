#include "OpcQL.h"
#include <jde/app/client/IAppClient.h>
#include <jde/app/client/awaits/LogSettingsClientAwait.h>
#include <jde/access/AccessException.h>
#include "../globals.h"

namespace Jde::Opc{
	sp<Server::OpcQL> _ql;
	α Server::QLPtr()ι->sp<OpcQL>{ ASSERT(_ql); return _ql; }
	α Server::QL()ι->OpcQL&{ return *QLPtr(); }
	α Server::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return QL().Schemas(); }
	α Server::ConfigureQL( sp<DB::AppSchema> schema, sp<Access::Authorize> authorizer )ι->void{
		QL::Configure( {schema} );
		_ql = ms<OpcQL>( move(schema), authorizer );
	}
}

namespace Jde::Opc::Server{
	OpcQL::OpcQL( sp<DB::AppSchema>&& schema, sp<Access::Authorize> authorizer )ι:
		App::AppQL{ {move(schema)}, move(authorizer) }{
		//QL::Configure is done once by ConfigureQL (the only construction site) before this runs; calling it here again was redundant.
	}
	//CustomMutation is ι, so a refusal has to be an await that throws on resume, not a throw.
	struct RefusedMutation final : TAwait<jvalue>{
		RefusedMutation( string reason, UserPK executer, EHttpStatus status, SL sl )ι:TAwait<jvalue>{sl}, _reason{move(reason)}, _executer{executer}, _status{status}{}
		α await_ready()ι->bool override{ return true; }
		α await_resume()ε->jvalue override{ throw Access::AccessException{ Source(), _executer, _status, "{}", _reason }; }
	private:
		α Suspend()ι->void override{ ASSERT(false); }
		string _reason;
		UserPK _executer;
		EHttpStatus _status;
	};

	α OpcQL::CustomQuery( QL::TableQL&, QL::Creds, SL )ι->up<TAwait<jvalue>>{return nullptr;}
	α OpcQL::CustomMutation( QL::MutationQL& m, QL::Creds creds, SL sl )ι->up<TAwait<jvalue>>{
		const auto executer = creds.UserPK();
		//the app server pushes updateLogSetting here when its instance_tag_levels rows change; without this route the push comes back an error.
		if( App::LogSettingsMAwait::IsApplicable(m) ){
			//...but only for a user.  That push carries the admin who ran updateInstanceTagLevel (InstanceTagLevelAwait's
			//pushRuntime -> QueryClient) and arrives over the app-client socket, so requiring one costs it nothing;  anonymous
			//over the web listener it rewrote the live SpdLog/ProtoLog levels and, `persist` defaulting on, had the AppServer
			//store them in instance_tag_levels under the OpcServer's own identity (opcserver-review3 #9).  Unauthorized rather
			//than Forbidden - it is about who is asking, which is the case AccessException documents the status argument for.
			//The same answer LogSettingsQuery's RequireAuthenticated gives the read side, deferred into an await because that
			//helper is ε and this is ι.
			return executer.Value
				? mu<App::Client::LogSettingsClientMAwait>( move(m), AppClient(), executer, sl )
				: up<TAwait<jvalue>>{ mu<RefusedMutation>(Ƒ("[{}]An authenticated user is required.", m.CommandName), executer, EHttpStatus::Unauthorized, sl) };
		}
		//The opc schema owns no tables at all now (its address space is NodeSet2 xml), so nothing below has a resource to gate
		//on and Authorize::Test would read every name as "not enabled - allow".  The schema sync's .mutation files (SchemaDdl,
		//UserPK::System) are the only ql writer;  everything else is a web request.
		return executer.Value==UserPK::System
			? nullptr
			: up<TAwait<jvalue>>{ mu<RefusedMutation>(Ƒ("'{}' is not supported;  the OpcServer's schema owns no tables to write.", m.CommandName), executer, EHttpStatus::Forbidden, sl) };
	}

	α OpcQL::LogSettingsQuery( QL::TableQL&& ql, QL::Creds executer, SL sl )ε->up<TAwait<jvalue>>{
		RequireAuthenticated( executer, "logSettings", sl );
		return mu<App::Client::LogSettingsClientAwait>( move(ql), sl );
	}
}