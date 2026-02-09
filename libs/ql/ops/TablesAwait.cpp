#include <jde/ql/ops/TablesAwait.h>
#include <jde/ql/IQL.h>
#include "SelectAwait.h"

#define let const auto
namespace Jde::QL{
	α TablesAwait::Execute()ι->TAwait<jvalue>::Task{
		optional<jvalue> y;
		try{
			for( auto&& table : _tables ){
				THROW_IF( table.Columns.empty() && table.Tables.empty(), "Table '{}' has no columns", table.ToString() );
				ASSERT( !_statement || _statement->From.Joins.size() );
				optional<jvalue> result;
				let returnRaw = table.ReturnRaw && _tables.size()==1;
				if( _ql ){
					if( table.JsonName=="status" )
						result = _ql->StatusQuery(move(table));
					else if( auto await = table.JsonName.starts_with("log") ? _ql->LogQuery(move(table), _sl) : nullptr; await )
						result = co_await *await;
					else if( auto await = _ql->CustomQuery(table, _creds, _sl); await )
						result = co_await *await;
				}
				if( !result ){
					if( _statement )
						result = co_await SelectAwait{ table, *_statement, _creds.UserPK(), true, _sl };
					else
						result = co_await SelectAwait{ table, _creds.UserPK(), true, _sl };
				}
				if( returnRaw )
					y = move( *result );
				else{
					if( !y )
						y = jobject{};
					y->get_object()[table.ReturnName()] = move( *result );
				}
			}
			Resume( y.value_or(jvalue{}) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}