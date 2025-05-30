#include "OdbcRow.h"
//#include <jde/framework/Assert.h>

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
	α OdbcRow::MoveString( uint i, SL sl )ε->string{ 
		auto sbinding = dynamic_cast<BindingString*>( _bindings[i].get() );
		THROW_IFSL( !sbinding, "BindingString not found for index={}", i );
		return sbinding->to_string();
	}
#define ASSRT ASSERTSL( i<_bindings.size(), sl )
	α OdbcRow::GetBit( uint i, SL sl )Ι->bool{ ASSRT; return _bindings[i]->GetBit(); }
	α OdbcRow::GetString( uint i, SL sl )Ι->string{ ASSRT; return _bindings[i]->to_string(); }
	α OdbcRow::GetInt( uint i, SL sl )Ι->int64_t{ ASSRT; return _bindings[i]->GetInt(); }
	α OdbcRow::GetInt32( uint i, SL sl )Ι->int32_t{ ASSRT; return _bindings[i]->GetInt32(); }
	α OdbcRow::GetIntOpt( uint i, SL sl )Ι->std::optional<_int>{ ASSRT; return _bindings[i]->GetIntOpt(); }
	α OdbcRow::GetDouble( uint i, SL sl )Ι->double{ ASSRT; return _bindings[i]->GetDouble(); }
	α OdbcRow::GetDoubleOpt( uint i, SL sl )Ι->std::optional<double>{ ASSRT; return _bindings[i]->GetDoubleOpt(); }
	α OdbcRow::GetTimePoint( uint i, SL sl )Ι->DBTimePoint{ ASSRT; return _bindings[i]->GetDateTime(); }
	α OdbcRow::GetTimePointOpt( uint i, SL sl )Ι->std::optional<DBTimePoint>{ ASSRT; return _bindings[i]->GetDateTimeOpt(); }
	α OdbcRow::GetUInt( uint i, SL sl )Ι->uint{ ASSRT; return _bindings[i]->GetUInt(); }
	α OdbcRow::GetUIntOpt( uint i, SL sl )Ι->std::optional<uint>{ ASSRT; return _bindings[i]->GetUIntOpt(); }
	
	α OdbcRow::Move()ι->up<IRow>{
		vector<Value> values;
		for( auto& binding : _bindings )
			values.emplace_back( binding->GetValue() );
		return mu<Row>( move(values) ); 
	}
	//TODO Actually move
	α OdbcRowMulti::Move()ι->up<IRow>{
		auto row = mu<OdbcRowMulti>( _bindings );
		row->_row = _row++;
		return row;
	}
	α OdbcRowMulti::MoveString( uint i, SL )ε->string{
		ASSERT( i<_bindings.size() ); 
		return _bindings[i]->MoveString( _row ); 
	}
}