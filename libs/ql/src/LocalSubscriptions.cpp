#include <jde/ql/LocalSubscriptions.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Sql.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/names.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/types/Parser.h>

#define let const auto
namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };
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

	//A mutation result only carries the row's id when the insert went through a proc; otherwise recover it so subscribers see the
	//same shape.  every failure here used to escape to OnMutation's catch _before_ the first
	//OnChange, so one awkward column cost *every* subscriber the notification.  Now a failed lookup costs the id field alone.
	Ω findId( const MutationQL& m, const jobject& args )ι->optional<uint>{
		let table = m.DBTable;
		let pk = table ? table->FindPK() : sp<DB::Column>{};
		if( !pk )
			return {};
		DB::WhereClause where; //the generator, not hand-rolled sql:  placeholders & qualified names then follow the driver's Syntax.
		for( let& [key, value] : args ){
			if( value.is_object() || value.is_array() || value.is_null() ) //a null resolves to `col is null`, which matches nothing when the column was server-defaulted (created/updated).
				continue;
			let column = table->FindColumn( DB::Names::FromJson(key) );
			if( !column || column==pk )
				continue;
			try{
				where.Add( column, DB::Value{column->Type, value} ); //Value{EType,jvalue} throws for Guid/VarBinary/TimeSpan/… and on json-kind mismatches - such a column just doesn't narrow the search.
			}
			catch( const std::exception& ){
				TRACE( "[{}.{}]'{}' does not convert to the column type - not narrowing the id lookup with it.", table->Name, column->Name, serialize(value) );
			}
		}
		if( where.Empty() )
			return {};
		optional<uint> y;
		try{
			vector<uint> ids;
			table->Schema->DS()->Select( DB::Sql{Ƒ("select {} from {} {}", pk->FQName(), table->DBName, where.ToString()), where.Params()}, [&ids](DB::Row&& r){ ids.push_back( r.GetUInt(0) ); } );
			if( ids.size()==1 )
				y = ids.front();
			else //0 rows was ScalerSync's throw; 2+ used to broadcast whichever row came last.
				WARN( "[{}]The id lookup matched {} rows for '{}' - notifying subscribers without an id.", table->Name, ids.size(), m.CommandName );
		}
		catch( const std::exception& e ){
			WARN( "[{}]The id lookup failed for '{}' ({}) - notifying subscribers without an id.", table->Name, m.CommandName, e.what() );
		}
		return y;
	}

	//comes from a mutation.
	α Subscriptions::OnMutation( const MutationQL& m, jvalue result )ι->void{
		OnMutation( m, move(result), nullptr );
	}
	α Subscriptions::OnMutation( const MutationQL& m, jvalue result, function<bool(QL::TableQL&)> isApplicable )ι->void{
		try{
			vector<ListenerSubs> matches;
			{
				sl l{ _serverMutex };
				auto subs = _serverSubs.find( {m.TableName(), m.Type} );
				if( subs==_serverSubs.end() )
					return;//everything is pushed.
				matches = subs->second;
			}//callbacks & db access below happen outside the lock - listeners can (un)subscribe from OnChange.
			jobject available;
			for( auto& sub : matches ){
				if( isApplicable && !isApplicable(sub.Fields) )
					continue;
				if( available.empty() ){
					if( let array = result.try_as_array(); array && array->size() )
						result = ( *array )[0];
					let args = m.ExtrapolateVariables();
					available = result.is_object() ? Json::Combine( args, result.get_object() ) : args;
					//match every scalar arg that maps to a column - target alone is ambiguous (e.g. resources rows differing only by criteria).
					//Args only: the result's fields are server-formatted (a `$now` default needn't byte-match what was written) and today carry nothing but id/rowCount anyway.
					if( !available.contains("id") ){
						if( let id = findId(m, args); id )
							available["id"] = *id;
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
		catch( const std::exception& ){
		}
	}

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