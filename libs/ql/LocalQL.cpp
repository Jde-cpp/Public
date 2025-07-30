#include <jde/ql/LocalQL.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalSubscriptions.h>
#include "ops/InsertAwait.h"

#define let const auto

namespace Jde::QL{
	α LocalQL::DS()ι->DB::IDataSource&{ ASSERT(!_schemas.empty() && _schemas.front()->DS()); return *_schemas.front()->DS(); }
	α LocalQL::GetTablePtr( str tableName, SL sl )ε->sp<DB::View>{
		for( const auto& schema : _schemas ){
			if( auto t = schema->FindView(tableName); t )
				return t;
		}
		throw Exception{ sl, "Table not found:  {}", tableName };
	}
	α LocalQL::GetTable( str tableName, SL sl )ε->DB::View&{
		return *GetTablePtr( tableName, sl );
	}

	constexpr ELogTags _tags{ ELogTags::QL };

	struct SubscribeQueryAwait : TAwait<vector<SubscriptionId>>{
		using Await = TAwait<jobject>;
		using base = TAwait<vector<SubscriptionId>>;
		SubscribeQueryAwait( vector<Subscription>&& sub, sp<IListener> listener, UserPK executer, SRCE )ι:
			base{sl}, _executer{executer}, _listener{listener}, _subscriptions{move(sub)}{}
		α await_ready()ι->bool override{
			for_each( _subscriptions, [&]( Subscription& sub ){_result.push_back(sub.Id);} );
			Subscriptions::Listen( _listener, move(_subscriptions) );
			return true;
		}
		α Suspend()ι->void override{}
		α await_resume()ι->vector<SubscriptionId> override{ return _result; }
	private:
		UserPK _executer;
		sp<IListener> _listener;
		vector<Subscription> _subscriptions;
		vector<SubscriptionId> _result;
	};
	α LocalQL::Subscribe( string&& query, sp<IListener> listener, UserPK executer, SL sl )ε->up<TAwait<vector<SubscriptionId>>>{
		Trace{ ELogTags::Test, "{}", query };
		return mu<SubscribeQueryAwait>( ParseSubscriptions(move(query), _schemas, sl), listener, executer, sl );
	}
	α LocalQL::Upsert( string query, UserPK executer )ε->jarray{
		auto result = QL::Parse( move(query), _schemas ); THROW_IF( !result.IsMutation(), "Query is not a mutation" );
		jarray y;
		for( auto&& m : result.Mutations() ){
			auto key = m.FindKey();
			if( !key ){
				let shift = m.GetParam( "shift" );
				key = { shift.is_null() ? 0 : 1ul << (Json::AsNumber<uint8>(shift)) };
				m.Args["id"] = key->PK();
				m.Args.erase( "shift" );
			}
			auto input = key->IsPrimary()
				? "id:"+std::to_string(key->PK())
				: "target:\""+move(key->NK())+'"';
			auto ql = Ƒ( "{}({}){{ id }}", DB::Names::ToSingular(m.JsonTableName), move(input) );
			if( auto existing = BlockAwait<TAwait<jobject>,jobject>(*QueryObject(move(ql), executer)); existing.empty() ){
				if( auto name = m.Args.contains("name") ? nullptr : m.Args.if_contains("target"); name ){
					string name2 = Json::AsString(*name);
					m.Args["name"] = name2;
				}
				if( auto t = key->IsPrimary() ? GetTablePtr(m.TableName()) : nullptr; t && t->SequenceColumn() )
					y.push_back( BlockAwait<InsertAwait,jvalue>({DB::AsTable(t), move(m), true, executer}) );
				else
					y.push_back( BlockAwait<QLAwait<jvalue>,jvalue>(QLAwait<jvalue>{move(m), executer}) );
			}else
				y.push_back( {} );
		}
		return y;
	}
}