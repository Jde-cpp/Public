#include <jde/web/client/socket/ClientQL.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/web/client/socket/IClientSocketSession.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/Subscription.h>
//#include <jde/ql/SubscriptionAwait.h>

#define let const auto

namespace Jde::Web::Client{
	struct SubscribeQueryAwait : TAwaitEx<vector<QL::SubscriptionId>,typename ClientSocketAwait<jarray>::Task>{
		using Await = ClientSocketAwait<jarray>;
		using base = TAwaitEx<vector<QL::SubscriptionId>,typename Await::Task>;
		SubscribeQueryAwait( Await&& await, SL sl )ι: base{sl}, _await{move(await)}{}
		α Execute()ι->Await::Task override{
			try{
				auto y = Json::FromArray<QL::SubscriptionId>( co_await _await );
				base::Resume( move(y) );
			}
			catch( IException& e ){
				base::ResumeExp( move(e) );
			}
		}
	private:
		Await _await;
	};

	α ClientQL::Subscribe( string&& query, sp<QL::IListener> listener, UserPK /*executer*/, SL sl )ε->up<TAwait<vector<QL::SubscriptionId>>>{
		auto await = _session->Subscribe( move(query), listener, sl );
		return mu<SubscribeQueryAwait>( move(await), sl );
	}

	Τ struct QueryAwait final : TAwaitEx<T,ClientSocketAwait<jvalue>::Task>{
		using Await = ClientSocketAwait<jvalue>;
		using base=TAwaitEx<T,Await::Task>;
		QueryAwait( string query, sp<IClientSocketSession> session, bool returnRaw, SRCE )ε:
			base{sl},_query{move(query)},_returnRaw{returnRaw},_session{session}{}
	private:
		α Execute()ι->Await::Task{
			try{
				Resume( co_await _session->Query(move(_query), _returnRaw, base::_sl) );
			}
			catch( IException& e ){
				base::ResumeExp( move(e) );
			}
		}
		α Resume( jvalue&& result )ι->void;
		string _query;
		bool _returnRaw;
		sp<IClientSocketSession> _session;
	};

	template<> Ξ QueryAwait<jvalue>::Resume( jvalue&& result )ι->void{ base::Resume( move(result) ); }
	template<> Ξ QueryAwait<jobject>::Resume( jvalue&& result )ι->void{
		if( result.is_object() )
			base::Resume( move(result.get_object()) );
		else if( result.is_null() )
			base::Resume( jobject{} );
		else
			ResumeExp( Exception{_sl, "Expected object."} );
	}
	template<> Ξ QueryAwait<jarray>::Resume( jvalue&& result )ι->void{
		if( result.is_array() )
			base::Resume( move(result.get_array()) );
		else
			ResumeExp( Exception{_sl, "Expected array."} );
	}


	α ClientQL::Query( string query, UserPK, bool returnRaw, SL sl )ι->up<TAwait<jvalue>>{
		return mu<QueryAwait<jvalue>>( move(query), _session, returnRaw, sl );
	}
	α ClientQL::QueryObject( string query, UserPK executer, bool returnRaw, SL sl )ε->up<TAwait<jobject>>{
		return mu<QueryAwait<jobject>>( move(query), _session, returnRaw, sl );
	}
	α ClientQL::QueryArray( string query, UserPK executer, bool returnRaw, SL sl )ε->up<TAwait<jarray>>{
		return mu<QueryAwait<jarray>>( move(query), _session, returnRaw, sl );
	}
}