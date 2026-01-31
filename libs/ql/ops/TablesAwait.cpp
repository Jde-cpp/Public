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
				jvalue result;
				let returnRaw = table.ReturnRaw && _tables.size()==1;
				if( auto await = _ql ? _ql->CustomQuery( table, _executer, _sl ) : nullptr; await )
					result = co_await *await;
				else{
					result = _statement
						? co_await SelectAwait{ table, *_statement, _executer, true, _sl }
						: co_await SelectAwait{ table, _executer, true, _sl };
				}
				if( returnRaw )
					y = move( result );
				else{
					if( !y )
						y = jobject{};
					y->get_object()[table.ReturnName()] = move( result );
				}
			}
			Resume( y.value_or( jvalue{} ) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}