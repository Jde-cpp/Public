//TablesAwait hands the table to the status/log handlers by rvalue, so the routing decision has to be made before the call and
//the moving branches have to be terminal - review #5 (use-after-move).  NullQL below is the pathological handler the old code
//trusted not to exist: it consumes the table and *still* returns null.  No data source is involved: the tables are `system`,
//so TableQL leaves _dbTable null, and Resume/ResumeExp are synchronous, so BlockAwait drives the coroutine on this thread.
#include <gtest/gtest.h>
#include <jde/ql/IQL.h>
#include <jde/ql/ops/TablesAwait.h>

#define let const auto

namespace Jde::QL::Tests{
	struct NullQL final : IQL{
		α Authorizer()ε->Access::Authorize& override{ throw Exception{"No authorizer."}; }
		α AuthorizerPtr()ε->sp<Access::Authorize> override{ return {}; }
		α CustomQuery( TableQL&, Creds, SL )ι->up<TAwait<jvalue>> override{ ++CustomQueryCount; return nullptr; }
		α CustomMutation( MutationQL&, Creds, SL )ι->up<TAwait<jvalue>> override{ return nullptr; }
		α LogQuery( TableQL&& ql, SL )ι->up<TAwait<jvalue>> override{ Consumed = mu<TableQL>( move(ql) ); ++LogQueryCount; return nullptr; }
		α LogSettingsQuery( TableQL&& ql, SL )ι->up<TAwait<jvalue>> override{ Consumed = mu<TableQL>( move(ql) ); ++LogSettingsQueryCount; return nullptr; }
		α StatusQuery( TableQL&& ql )ι->jobject override{ Consumed = mu<TableQL>( move(ql) ); return jobject{ {"up",true} }; }
		α Query( string, jobject, UserPK, bool, SL )ε->up<TAwait<jvalue>> override{ return nullptr; }
		α QueryObject( string, jobject, UserPK, bool, SL )ε->up<TAwait<jobject>> override{ return nullptr; }
		α QueryArray( string, jobject, UserPK, bool, SL )ε->up<TAwait<jarray>> override{ return nullptr; }
		α Subscribe( string&&, jobject, sp<IListener>, UserPK, SL )ε->up<TAwait<vector<SubscriptionId>>> override{ return nullptr; }
		α Upsert( string, jobject, UserPK )ε->jarray override{ return {}; }
		α Schemas()Ι->const vector<sp<DB::AppSchema>>& override{ return _schemas; }

		up<TableQL> Consumed;
		uint LogQueryCount{};
		uint LogSettingsQueryCount{};
		uint CustomQueryCount{};
	private:
		const vector<sp<DB::AppSchema>> _schemas; //empty:  nothing here opens a data source.
	};

	Ω tables( sv jsonName )ε->vector<TableQL>{
		const vector<sp<DB::AppSchema>> noSchemas;
		vector<TableQL> y;
		y.emplace_back( string{jsonName}, jobject{}, ms<jobject>(), noSchemas, true );
		return y;
	}
	Ω execute( sv jsonName, sp<NullQL> ql )ε->jvalue{
		return BlockAwait<TablesAwait,jvalue>( TablesAwait{tables(jsonName), {}, Creds{UserPK{UserPK::System}}, sp<IQL>{ql}, SRCE_CUR} );
	}

	//A handler that consumed the table and returned null used to fall through to the next branch/the generic select with a
	//gutted TableQL; now it is an error naming the table it was asked for.
	TEST( TablesAwaitTests, LogQueryReturningNullThrowsNamingTheTable ){
		auto ql = ms<NullQL>();
		try{
			execute( "logs", ql );
			FAIL() << "expected a throw";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("logs"), string::npos ); //not the moved-from (empty) name.
		}
		EXPECT_EQ( ql->LogQueryCount, 1u );
		EXPECT_EQ( ql->CustomQueryCount, 0u ); //no fallback ran on the moved-from table.
		ASSERT_TRUE( ql->Consumed );
		EXPECT_EQ( ql->Consumed->JsonName, "logs" );
	}

	//"logSettings" starts with "log", so the routing has to be settled before LogSettingsQuery takes ownership - reading
	//table.JsonName afterwards decided the LogQuery branch on a moved-from string.
	TEST( TablesAwaitTests, LogSettingsQueryReturningNullDoesNotRetryAsLogQuery ){
		auto ql = ms<NullQL>();
		EXPECT_THROW( execute("logSettings", ql), Exception );
		EXPECT_EQ( ql->LogSettingsQueryCount, 1u );
		EXPECT_EQ( ql->LogQueryCount, 0u );
		EXPECT_EQ( ql->CustomQueryCount, 0u );
	}

	//logLevels is a real table:  the log prefixes must not swallow it, so it reaches CustomQuery with the table intact.
	TEST( TablesAwaitTests, LogLevelsIsNotALogQuery ){
		auto ql = ms<NullQL>();
		EXPECT_THROW( execute("logLevels", ql), Exception ); //no schema -> the select fallback has no table.
		EXPECT_EQ( ql->LogQueryCount, 0u );
		EXPECT_EQ( ql->LogSettingsQueryCount, 0u );
		EXPECT_EQ( ql->CustomQueryCount, 1u );
		EXPECT_FALSE( ql->Consumed );
	}

	//StatusQuery returns by value, so its result is always set - the select fallback can never see the table it moved.
	TEST( TablesAwaitTests, StatusQueryResultShortCircuitsTheSelect ){
		auto ql = ms<NullQL>();
		let y = execute( "status", ql );
		EXPECT_EQ( Json::AsBool(y.get_object(),"up"), true );
		EXPECT_EQ( ql->CustomQueryCount, 0u );
		ASSERT_TRUE( ql->Consumed );
		EXPECT_EQ( ql->Consumed->JsonName, "status" );
	}
}
