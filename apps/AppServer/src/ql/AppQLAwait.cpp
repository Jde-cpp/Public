#include "AppQLAwait.h"
#include "ConnectionQLAwait.h"
#include <jde/ql/QLAwait.h>

#define let const auto

namespace Jde::App::Server{
	α AppQLAwait::Execute()ι->TAwait<jvalue>::Task{
		try{
			if( _ql.IsQueries() ){
				_raw = _raw && _ql.Queries().size()==1;
				jvalue y = _raw ? jvalue{} : jobject{};
				for( auto& q : _ql.Queries() ){
					q.ReturnRaw = true;
					let memberName = q.ReturnName();
					jvalue queryResult;
					if( q.JsonName.starts_with("connection") )
						queryResult = co_await ConnectionQLAwait{ move(q), _executer, _sl };
					else
						queryResult = co_await QL::QLAwait( move(q), _executer, _sl );
					if( _raw )
						y = queryResult;
					else
						y.get_object()[memberName] = move( queryResult );
				}
				Resume( move(y) );
			}
			else
				throw Exception{ _sl, Jde::ELogLevel::Debug, "Only queries are supported." };
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}