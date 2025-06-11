#include <jde/db/generators/Object.h>
#include <jde/framework/str.h>
#include <jde/db/generators/Coalesce.h>

#pragma GCC diagnostic ignored "-Wswitch"

namespace Jde {
	α DB::GetParams( Object& o )ι->vector<DB::Value>{
		vector<DB::Value> params;
		switch( (EObject)o.index() ){
			using enum EObject;
			case Value: params.push_back(move(get<DB::Value>(o))); break;
			case Values: params = move(get<vector<DB::Value>>(o)); break;
			case Coalesce: params = move(get<DB::Coalesce>(o).Params()); break;
		}
		return params;
	};

	α DB::ToString( Object& o )ι->string{
		switch( (EObject)o.index() ){
			using enum EObject;
			case Column: return get<sp<DB::Column>>(o)->FQName();
			case AliasColumn: return get<AliasCol>(o).ToString();
			case Value: return "?";
			case Coalesce: return get<DB::Coalesce>(o).ToString();
			case Values:{
				const auto& values = get<vector<DB::Value>>(o);
				string s( "[" ); s.reserve( values.size() * 10 );
				for( const auto& v : values )
					s += ' '+v.ToString()+',';
				s.back() = ']';
				return s;
			}
		}
		return {};
	}
}