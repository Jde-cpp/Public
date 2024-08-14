#include <jde/web/server/SessionGraphQL.h>
#include "../../../../Framework/source/db/GraphQL.h"
#include "../../../../Framework/source/io/ProtoUtilities.h"
#include <jde/io/Json.h>
#include <jde/web/server/Flex.h>
#include <jde/web/server/Sessions.h>

#define var const auto

namespace Jde::Web::Server{
	constexpr ELogTags _tags{ ELogTags::Sessions };

	//query{ session(filter:{ id:${a0b1cef2} }){ domain loginName } }
	α Select( DB::TableQL query, UserPK executerPK, HCoroutine h, SL sl )ι->TAwait<json>::Task{
		try{
			var sessionString = Json::Get<string>( query.Args, "id", ELogTags::HttpServerRead, sl );
			var sessionId = sessionString.empty() ? nullopt : Str::TryTo<SessionPK>(sessionString, nullptr, 16 );
			if( !sessionString.empty() && !sessionId )
				co_return Resume( Exception(_tags, sl, "Could not parse sessionid: '{}'", sessionString), h );
			vector<sp<Server::SessionInfo>> sessions;
			if( sessionString.empty() )
				sessions = Sessions::Get();
			else if( auto p = Sessions::Find(*sessionId); p )
				sessions.push_back( p );
			if( sessions.empty() )
				co_return Resume( mu<json>(), h ); //(Json::Parse( "{\"data\": null}"))
			flat_map<UserPK, tuple<string,string>> userDomainLoginNames;
			json users;
			if( query.FindColumn("domain") || query.FindColumn("loginName") ){
				string inClause; inClause.reserve( sessions.size()*5 );
				for( var& session : sessions )
					inClause += std::to_string( session->UserPK ) + ",";
				auto q = "query{ users(id:["+inClause.substr(0, inClause.size()-1)+"]){id loginName provider{id name}} }";
				users = co_await (*AppGraphQLAwait(move(q), executerPK) );
				Trace( _tags | ELogTags::Pedantic, "users={}"sv, users.dump() );
				for( var& user : users["data"]["users"] )
					userDomainLoginNames[Json::Get<UserPK>(user,"id")] = make_tuple( Json::Getε(user,std::vector<sv>{"provider","name"}), Json::Get(user,"loginName") );
			}
			auto y = query.IsPlural() ? mu<json>(json::array()) : mu<json>(json::object());
			for( var& session : sessions ){
				json j = json::object();
				if( auto pUser = userDomainLoginNames.find(session->UserPK); pUser!=userDomainLoginNames.end() ){
					if( query.FindColumn("domain") )
						j["domain"] = std::get<0>(pUser->second);
					if( query.FindColumn("loginName") )
						j["loginName"] = std::get<1>(pUser->second);
				}
#ifndef NDEBUG
				if( query.FindColumn("id") )
					j["id"] = 𐢜( "{:x}", session->SessionId );
#endif
				if( query.FindColumn("endpoint") )
					j["endpoint"] = session->UserEndpoint;
				if( query.FindColumn("endpoint") )
					j["lastUpdate"] = DateTime{ session->Expiration }.ToIsoString();
				if( query.FindColumn("expiration") )
					j["expiration"] = DateTime{ session->Expiration }.ToIsoString();
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