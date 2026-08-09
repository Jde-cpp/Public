#include <jde/ql/ops/TablesAwait.h>
#include <jde/ql/IQL.h>
#include "SelectAwait.h"

#define let const auto
namespace Jde::QL{
	α TablesAwait::Execute()ι->TAwait<jvalue>::Task{
		optional<jvalue> y;
		try{
			for( auto&& table : _tables ){
				ASSERT( !_statement || _statement->From.Joins.size() );
				optional<jvalue> result;
				let returnRaw = table.ReturnRaw && _tables.size()==1;
				auto returnName = table.ReturnName();
				if( _ql ){
					//route on the name _before_ handing the table over:  StatusQuery/LogSettingsQuery/LogQuery take it by rvalue, so nothing
					//after the call may read `table` (or a reference into it) again - hence the branches that move are terminal.
					let& jsonName = table.JsonName;
					let isStatus = jsonName=="status";
					let isLogSettings = jsonName.starts_with( "logSetting" );
					let isLog = !isLogSettings && jsonName.starts_with( "log" ) && !jsonName.starts_with( "logLevel" );
					if( isStatus )
						result = _ql->StatusQuery( move(table) ); //returns by value - always sets result, so the select fallback below can't see the moved-from table.
					else if( isLogSettings || isLog ){
						auto await = isLogSettings ? _ql->LogSettingsQuery( move(table), _sl ) : _ql->LogQuery( move(table), _sl );
						THROW_IF( !await, "[{}]{} returned null.", returnName, isLogSettings ? "LogSettingsQuery" : "LogQuery" ); //the table is gone - there is nothing left to fall back on.
						result = co_await *await;
					}
					else if( auto await = _ql->CustomQuery( table, _creds, _sl ); await ) //takes an lvalue: ownership stays here.
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
					y->get_object()[move(returnName)] = move( *result );
				}
			}
			Resume( y.value_or(jvalue{}) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}