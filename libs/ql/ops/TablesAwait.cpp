#include <jde/ql/ops/TablesAwait.h>
#include "SelectAwait.h"

#define let const auto
namespace Jde::QL{
	α TablesAwait::Execute()ι->TAwait<jvalue>::Task{
		optional<jvalue> y;
		try{
			for( let& table : _tables ){
				if( table.Columns.empty() && table.Tables.empty() )
					throw Jde::Exception{ _sl, "Table '{}' has no columns", table.ToString() };
				ASSERT( !_statement || _statement->From.Joins.size() );
				auto result = _statement
					? co_await SelectAwait{ table, *_statement, _executer, true, _sl }
					: co_await SelectAwait{ table, _executer, true, _sl };
				if( table.ReturnRaw && _tables.size()==1 )
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