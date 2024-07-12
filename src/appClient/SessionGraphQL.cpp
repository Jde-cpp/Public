#include <jde/appClient/SessionGraphQL.h>
#include <jde/appClient/Sessions.h>
#include <jde/io/Json.h>
#include <jde/appClient/AppClient.h>
#include "../../../Framework/source/db/GraphQL.h"
//#include "../../../Framework/source/io/ServerSink.h"
#include "../../../Framework/source/io/ProtoUtilities.h"

#define var const auto

namespace Jde::Web{
	using namespace Jde::App::Client;
	static sp<LogTag> _logTag = Logging::Tag( "web.sessions" );
	//using DB::GraphQL::Hook::Operation;

	//query{ session(filter:{ id:{eq:${this.sessionId}} }){ domain loginName } }
	α Select( DB::TableQL query, UserPK executerPK, HCoroutine h, SL sl )ι->GraphQLAwait::Task{
		try{
			var sessionId = Json::Get<SessionPK>( query.Args, "id", sl );
			vector<SessionInfo> sessions;
			if( !sessionId )
				sessions = GetSessions();
			else if( auto p = FindSession(sessionId); p )
				sessions.push_back( *p );
			if( sessions.empty() )
				co_return Resume( mu<json>(), h ); //(Json::Parse( "{\"data\": null}"))
			flat_map<UserPK, tuple<string,string>> userDomainLoginNames;
			json users;
			if( query.FindColumn("domain") || query.FindColumn("loginName") ){
				string inClause; inClause.reserve( sessions.size()*5 );
				for( var& session : sessions )
					inClause += std::to_string( session.UserPK ) + ",";
				auto q = "query{ users(id:["+inClause.substr(0, inClause.size()-1)+"]){id loginName provider{id name}} }";
				users = co_await GraphQLAwait( move(q) );
				//await( json, Logging::IServerSink::GraphQL("query{ users(id:["+inClause.substr(0, inClause.size()-1)+"]){id loginName provider{id name}} }", executerPK, sl) );
				DBG( "users={}"sv, users.dump() );
				for( var& user : users["data"]["users"] )
					userDomainLoginNames[Json::Get<UserPK>(user,"id")] = make_tuple( Json::Getε(user,std::vector<sv>{"provider","name"}), Json::Get(user,"loginName") );
			}
			auto y = query.IsPlural() ? mu<json>(json::array()) : mu<json>(json::object());
			for( var& session : sessions ){
				json j = json::object();
				if( auto pUser = userDomainLoginNames.find(session.UserPK); pUser!=userDomainLoginNames.end() ){
					if( query.FindColumn("domain") )
						j["domain"] = std::get<0>(pUser->second);
					if( query.FindColumn("loginName") )
						j["loginName"] = std::get<1>(pUser->second);
				}
				if( query.FindColumn("expiration") )
					j["expiration"] = DateTime{ session.Expiration }.ToIsoString();
				if( query.IsPlural() )
					y->push_back( j );
				else
					*y = move( j );
			}
			Resume( move(y), h );
		}
		catch( IException& e ){
			Resume( move(e), h );
		}
	}

	struct SessionGraphQLAwait final: AsyncAwait{
		SessionGraphQLAwait( const DB::TableQL& query, UserPK userPK_, SRCE )ι:
			AsyncAwait{
				[&, userPK=userPK_]( HCoroutine h ){ Select( query, userPK, h, _sl ); },
				_sl, "WebGraphQLAwait" }
		{}
	};

	α SessionGraphQL::Select( const DB::TableQL& query, UserPK userPK, SL sl )ι->up<IAwait>{
		return query.JsonName.starts_with( "session" ) ? mu<SessionGraphQLAwait>( query, userPK, sl ) : nullptr;
	}
}