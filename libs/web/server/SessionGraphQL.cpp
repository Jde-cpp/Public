#include <jde/web/server/SessionGraphQL.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/TableQL.h>
#include <jde/framework/io/proto.h>
#include <jde/framework/io/json.h>
#include <jde/web/server/Sessions.h>
#include "ServerImpl.h"

#define let const auto

namespace Jde::Web::Server{
	constexpr ELogTags _tags{ ELogTags::Sessions };

	α Select( QL::TableQL query, UserPK executerPK, HCoroutine h, SL sl )ι->TAwait<jobject>::Task{
		try{
			let sessionString = Json::FindString( query.Args, "id" );
			let sessionId = sessionString ? Str::TryTo<SessionPK>(*sessionString, nullptr, 16 ) : nullopt;
			if( sessionString && !sessionId )
				co_return Resume( Exception(_tags, sl, "Could not parse sessionid: '{}'", *sessionString), h );
			vector<sp<Server::SessionInfo>> sessions;
			if( !sessionString )
				sessions = Sessions::Get();
			else if( auto p = Sessions::Find(*sessionId); p )
				sessions.push_back( p );
			if( sessions.empty() )
				co_return Resume( mu<jobject>(), h ); //(Json::Parse( "{\"data\": null}"))
			flat_map<UserPK, tuple<string,string>> userDomainLoginNames;
			jobject users;
			if( query.FindColumn("domain") || query.FindColumn("loginName") ){
				string inClause; inClause.reserve( sessions.size()*5 );
				for( let& session : sessions )
					inClause += std::to_string( session->UserPK ) + ",";
				auto q = "query{ users(id:["+inClause.substr(0, inClause.size()-1)+"]){id loginName provider{id name}} }";
				users = co_await (*AppGraphQLAwait(move(q), executerPK) );
				Trace( _tags | ELogTags::Pedantic, "users={}"sv, serialize(users) );
				for( let& vuser : Json::AsArrayPath(users, "data/users") ){
					let& user = Json::AsObject(vuser);
					userDomainLoginNames[Json::AsNumber<UserPK>(user,"id")] = make_tuple( Json::AsSVPath(user, "provider/name"), Json::AsString(user, "loginName") );
				}
			}
			auto array = query.IsPlural() ? jarray{} : optional<jarray>{};
			for( let& session : sessions ){
				jobject j;
				if( auto pUser = userDomainLoginNames.find(session->UserPK); pUser!=userDomainLoginNames.end() ){
					if( query.FindColumn("domain") )
						j["domain"] = std::get<0>(pUser->second);
					if( query.FindColumn("loginName") )
						j["loginName"] = std::get<1>(pUser->second);
				}
#ifndef NDEBUG
				if( query.FindColumn("id") )
					j["id"] = Ƒ( "{:x}", session->SessionId );
#endif
				if( query.FindColumn("endpoint") )
					j["endpoint"] = session->UserEndpoint;
				if( query.FindColumn("endpoint") )
					j["lastUpdate"] = DateTime{ session->Expiration }.ToIsoString();
				if( query.FindColumn("expiration") )
					j["expiration"] = DateTime{ session->Expiration }.ToIsoString();
				if( array )
					array->emplace_back( move(j) );
				else{
					Resume( mu<jobject>(move(j)), h );
					break;
				}
			}
			if( array )
				Resume( mu<jarray>(move(*array)), h );
		}
		catch( IException& e ){
			Resume( move(e), h );
		}
	}

	struct SessionGraphQLAwait final: AsyncAwait{
		SessionGraphQLAwait( const QL::TableQL& query, UserPK userPK_, SRCE )ι:
			AsyncAwait{
				[&, userPK=userPK_]( HCoroutine h ){ Select( query, userPK, h, _sl ); },
				sl, "WebGraphQLAwait" }
		{}
	};

	α SessionGraphQL::Select( const QL::TableQL& query, UserPK userPK, SL sl )ι->up<IAwait>{
		return query.JsonName.starts_with( "session" ) ? mu<SessionGraphQLAwait>( query, userPK, sl ) : nullptr;
	}
}