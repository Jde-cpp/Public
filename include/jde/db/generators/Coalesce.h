#include "../Value.h"
#include "../meta/Column.h"
#include "Object.h"

namespace Jde::DB{
	struct Coalesce final{
		Coalesce( Object&& a, Object&& b )ι{
			Objects.push_back( move(a) );
			Objects.push_back( move(b) );
		}
		α ToString()ι->string{
			string s{ "coalesce(" }; s.reserve( 64 );
			for( auto&& o : Objects ){
				if( o.index()==underlying(EObject::Value) )
					s+= " ?,";
				else
					s += ' '+DB::ToString(o)+',';
			}
			s.back() = ')';
			return s;
		}
		α Params()ι->vector<Value>{
			vector<Value> params;
			for( auto&& o : Objects ){
				if( o.index()==underlying(EObject::Value) )
					params.push_back( move(get<Value>(o)) );
			}
			return params;
		}
		vector<Object> Objects;
	};
}