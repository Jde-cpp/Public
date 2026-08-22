//review3 #6:  TablesAwait routes logs/logSettings/status on the json name, around SelectAwait's Authorize, and the three IQL
//overloads it routes to took no credentials at all - so the log archive, the live log configuration and the status document
//answered an anonymous /graphql request.  They now take Creds like CustomQuery does, and AppQL::RequireAuthenticated is the
//gate.  Nothing here starts a server or reads an archive:  LogQLAwait's constructor only stores the query.
#include <gtest/gtest.h>
#include <jde/app/AppQL.h>
#include "helpers.h"

#define let const auto

namespace Jde::App::Tests{
	//AppQL is abstract (CustomQuery/CustomMutation/LogSettingsQuery stay pure);  LogSettingsQuery mirrors what the three apps do.
	struct QLStub final : AppQL{
		QLStub()ι:AppQL{ {}, {} }{}
		α CustomQuery( QL::TableQL&, QL::Creds, SL )ι->up<TAwait<jvalue>> override{ return nullptr; }
		α CustomMutation( QL::MutationQL&, QL::Creds, SL )ι->up<TAwait<jvalue>> override{ return nullptr; }
		α LogSettingsQuery( QL::TableQL&& , QL::Creds executer, SL sl )ε->up<TAwait<jvalue>> override{
			RequireAuthenticated( executer, "logSettings", sl );
			return nullptr;
		}
	};
	Ω anonymous()ι->QL::Creds{ return QL::Creds{ UserPK{} }; } //what Sessions::CreateSession hands a request with no Authorization header.
	Ω user()ι->QL::Creds{ return QL::Creds{ UserPK{7} }; }

	//the UI's own logs shape, as an anonymous GET /graphql would arrive.
	TEST( AppQLTests, LogQueryRefusesAnAnonymousCaller ){
		auto ql = ms<QLStub>();
		auto logs = table( "logs", "{limit:1}" );
		addColumns( addTable(logs, "entries", {}), {"level"} );
		try{
			ql->LogQuery( move(logs), anonymous(), SRCE_CUR );
			FAIL() << "expected a throw";
		}
		catch( const Exception& e ){
			EXPECT_EQ( e.HttpStatus(), EHttpStatus::Unauthorized ); //401, not a 500 - the status rides the base to the client.
			EXPECT_NE( string{e.what()}.find("logs"), string::npos ) << e.what();
		}
	}
	TEST( AppQLTests, LogQueryStillAnswersAnAuthenticatedCaller ){
		auto ql = ms<QLStub>();
		EXPECT_TRUE( ql->LogQuery(table("logs","{limit:1}"), user(), SRCE_CUR) ); //non-null LogQLAwait, nothing read yet.
	}

	TEST( AppQLTests, StatusQueryRefusesAnAnonymousCaller ){
		auto ql = ms<QLStub>();
		EXPECT_THROW( ql->StatusQuery(table("status"), anonymous(), SRCE_CUR), Exception );
	}
	TEST( AppQLTests, LogSettingsQueryRefusesAnAnonymousCaller ){
		auto ql = ms<QLStub>();
		EXPECT_THROW( ql->LogSettingsQuery(table("logSettings"), anonymous(), SRCE_CUR), Exception );
		EXPECT_NO_THROW( ql->LogSettingsQuery(table("logSettings"), user(), SRCE_CUR) );
	}
}
