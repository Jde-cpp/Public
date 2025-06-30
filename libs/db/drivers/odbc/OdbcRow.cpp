#include "OdbcRow.h"

namespace Jde::DB::Odbc{
	OdbcRow::OdbcRow( const vector<up<Binding>>& bindings )ι:
		_bindings{ bindings },
		_values{ bindings.size() }
	{}

	α OdbcRow::operator[]( uint index )Ι->const Value&{ return const_cast<OdbcRow*>(this)->operator[](index); }
	α OdbcRow::operator[]( uint index )ι->Value&{
		for( uint i = _values.size(); i <= index; ++i )
			_values.emplace_back( nullopt );
		if (!_values[index])
			_values[index] = _bindings[index]->GetValue();
		return *_values[index];
	}
	α OdbcRow::MoveString( uint i )ε->string{
		auto sbinding = dynamic_cast<BindingString*>( _bindings[i].get() );
		THROW_IF( !sbinding, "BindingString not found for index={}", i );
		return sbinding->to_string();
	}
#define ASSRT ASSERT( i<_bindings.size(), sl )
	α OdbcRow::GetBit( uint i )Ι->bool{ ASSRT; return _bindings[i]->GetBit(); }
	α OdbcRow::GetString( uint i )Ι->string{ ASSRT; return _bindings[i]->to_string(); }
	α OdbcRow::GetInt( uint i )Ι->int64_t{ ASSRT; return _bindings[i]->GetInt(); }
	α OdbcRow::GetInt32( uint i )Ι->int32_t{ ASSRT; return _bindings[i]->GetInt32(); }
	α OdbcRow::GetIntOpt( uint i )Ι->std::optional<_int>{ ASSRT; return _bindings[i]->GetIntOpt(); }
	α OdbcRow::GetDouble( uint i )Ι->double{ ASSRT; return _bindings[i]->GetDouble(); }
	α OdbcRow::GetDoubleOpt( uint i )Ι->std::optional<double>{ ASSRT; return _bindings[i]->GetDoubleOpt(); }
	α OdbcRow::GetTimePoint( uint i )Ι->DBTimePoint{ ASSRT; return _bindings[i]->GetDateTime(); }
	α OdbcRow::GetTimePointOpt( uint i )Ι->std::optional<DBTimePoint>{ ASSRT; return _bindings[i]->GetDateTimeOpt(); }
	α OdbcRow::GetUInt( uint i )Ι->uint{ ASSRT; return _bindings[i]->GetUInt(); }
	α OdbcRow::GetUIntOpt( uint i )Ι->std::optional<uint>{ ASSRT; return _bindings[i]->GetUIntOpt(); }

	α OdbcRow::ToRow()ι->Row{
		vector<Value> values;
		for( auto& binding : _bindings )
			values.emplace_back( binding->GetValue() );
		return { move(values) };
	}

	α OdbcRowMulti::MoveString( uint i )ε->string{
		ASSERT( i<_bindings.size() );
		return _bindings[i]->MoveString( _row );
	}
}