#include <jde/ql/LocalSubscriptions.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/types/MutationQL.h>
#include "types/Parser.h"

#define let const auto
namespace Jde::QL{
	struct TableOp final{
		TableOp( string tableName, EMutationQL type )ι: TableName{ move(tableName) }, Type{ type }{}
		string TableName;
		EMutationQL Type;
		α operator<( const TableOp& rhs )Ι->bool{
			return Type==rhs.Type ? TableName<rhs.TableName : Type<rhs.Type;
		}
	};
	struct ListenerSubs final{
		ListenerSubs( SubscriptionId id, TableQL&& fields, sp<IListener> listener )ι: Id{ id }, Fields{ move(fields) }, Listener{ listener }{}
		SubscriptionId Id;
		TableQL Fields;
		sp<IListener> Listener;
	};
	flat_map<TableOp,vector<ListenerSubs>> _serverSubs; std::shared_mutex _serverMutex;

	//comes from a mutation.
	α Subscriptions::OnMutation( const MutationQL& m, jvalue result )ι->void{
		sl l{ _serverMutex };
		auto subs = _serverSubs.find( {m.TableName(), m.Type} );
		if( subs==_serverSubs.end() )
			return;//everything is pushed.
		jobject available;
		for( auto& sub : subs->second ){
			if( available.empty() ){
				if( let array = result.try_as_array(); array && array->size() )
					result = (*array)[0];
				available = result.is_object() ? Json::Combine( m.ExtrapolateVariables(), result.get_object() ) : m.ExtrapolateVariables();
				if( !available.contains("id") && m.DBTable && m.DBTable->FindPK() ){
					available["id"] = m.DBTable->Schema->DS()->ScalerSync<uint>(
						{ Ƒ("select {} from {} where target=?", m.DBTable->FindPK()->Name, m.DBTable->DBName), {{string{available.at("target").as_string()}}} }
					);
				}
			}
			jobject j;
			auto value = sub.Fields.TrimColumns( available );
			j[sub.Fields.JsonName] = move( value );
			try{
				sub.Listener->OnChange( j, sub.Id );
			}
			catch( std::exception& )
			{}
		}
	}

/*
	α Subscriptions::Add( vector<Subscription>&& subs )ι->jarray{
		jarray y;
		ul _{ _mutex };
		for( auto&& s : subs ){
			s.Id = ++_id;
			y.emplace_back( s.Id );
			_subscriptions.try_emplace( s.TableName ).first->second.emplace_back( move(s) );
		}
		return y;
	}
*/
	α Subscriptions::StopListen( sp<IListener> listener, vector<SubscriptionId> ids )ι->jarray{
		jarray y;
		ul _{ _serverMutex };
		for( auto tableOp = _serverSubs.begin(); tableOp!=_serverSubs.end(); ){ //TableOp,vector<ListenerSubs>
			auto&& listenerSubs = tableOp->second;
			for( auto listenerSub = listenerSubs.begin(); listenerSub!=listenerSubs.end(); ){
				if( listenerSub->Listener==listener && (ids.empty() || find(ids, listenerSub->Id)!=ids.end()) ){
					y.push_back( listenerSub->Id );
					listenerSub = listenerSubs.erase( listenerSub );
				}else
					++listenerSub;
			}
			tableOp = listenerSubs.empty() ? _serverSubs.erase( tableOp ) : next( tableOp );
		}
		return y;
	}
	α Subscriptions::Listen( sp<IListener> listener, vector<Subscription>&& subs )ι->void{
		ul _{ _serverMutex };
		for( auto&& s : subs ){
			TRACET( ELogTags::QL, "[{}]Listen:  '{}'.'{}'", listener->Name, s.TableName, ToString(s.Type) );
			_serverSubs.try_emplace( {move(s.TableName), s.Type} ).first->second.emplace_back( ListenerSubs{s.Id,move(s.Fields), listener} );
		}
	}
}