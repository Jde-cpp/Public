#include "OdbcRow.h"

namespace Jde::DB::Odbc{
	α ToRow( const vector<up<Binding>>& bindings )ι->Row{
		vector<Value> values; values.reserve( bindings.size() );
		for( auto& binding : bindings )
			values.emplace_back( binding->GetValue() );
		return { move(values) };
	}
}