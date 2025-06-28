#include <jde/db/generators/Functions.h>

#define let const auto
namespace Jde::DB{
	Coalesce::Coalesce( Object&& a, Object&& b )ι{
		Objects.push_back( move(a) );
		Objects.push_back( move(b) );
	}
	α Coalesce::ToString()Ι->string{
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
	α Coalesce::Params()ι->vector<Value>{
		vector<Value> params;
		for( auto&& o : Objects ){
			if( o.index()==underlying(EObject::Value) )
				params.push_back( move(get<Value>(o)) );
		}
		return params;
	}
	α Coalesce::Params()Ι->vector<Value>{
		vector<Value> params;
		for( let& o : Objects ){
			if( o.index()==underlying(EObject::Value) )
				params.push_back( get<Value>(o) );
		}
		return params;
	}
}