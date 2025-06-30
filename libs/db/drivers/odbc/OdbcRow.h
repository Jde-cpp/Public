#pragma once
#include <jde/db/Row.h>
#include "Binding.h"
#include "Bindings.h"

namespace Jde::DB::Odbc{
	struct OdbcRow{
		OdbcRow( const vector<up<Binding>>& bindings )ι;
		α Reset()Ι->void{_index=0;}
		α ToRow()ι->Row;
		α operator[]( uint value )Ι->const Value&;
		α operator[]( uint i )ι->Value&;
		α GetBit( uint position )Ι->bool;
		α MoveString( uint i )ε->string;
		α GetString( uint position )Ι->string;
		int64_t GetInt( uint position )Ι;
		int32_t GetInt32( uint position )Ι;
		std::optional<_int> GetIntOpt( uint position )Ι;
		double GetDouble( uint position )Ι;
		float GetFloat( uint position )Ι{ return static_cast<float>( GetDouble(position) ); }
		std::optional<double> GetDoubleOpt( uint position )Ι;
		DBTimePoint GetTimePoint( uint position )Ι;
		std::optional<DBTimePoint> GetTimePointOpt( uint position )Ι;
		uint GetUInt( uint position )Ι;
		α GetUIntOpt( uint position )Ι->std::optional<uint>;
		α Size()Ι->uint { return _bindings.size(); }
	private:
		mutable uint _index{};
		const vector<up<Binding>>& _bindings;
		vector<optional<Value>> _values;
	};

	struct OdbcRowMulti{
		OdbcRowMulti( const vector<up<IBindings>>& bindings )ι:_bindings{bindings}{};
		void Reset()Ι{ _index=0; ++_row; }
		void ResetRowIndex()Ι{ _row=0; }

		α operator[]( uint position )Ι->const Value&{ ASSERT( position<_bindings.size() );  return _bindings[position]->Object(_row); }
		α operator[]( uint i )ι->Value&{ ASSERT( i<_bindings.size() );  return _bindings[i]->Object(_row); }
		α GetBit( uint position )Ι->bool{ ASSERT( position<_bindings.size() ); return _bindings[position]->Bit(_row); }
		α GetString( uint position )Ι->string{ ASSERT( position<_bindings.size() ); return _bindings[position]->ToString(_row); }
		α MoveString( uint i )ε->string;
		int64_t GetInt( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->Int(_row); }
		int32_t GetInt32( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->Int32(_row); }
		std::optional<_int> GetIntOpt( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->IntOpt(_row); }
		double GetDouble( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->Double(_row); }
		float GetFloat( uint position )Ι{ return static_cast<float>( GetDouble(position) ); }
		std::optional<double> GetDoubleOpt( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->DoubleOpt(_row); }
		DBTimePoint GetTimePoint( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->DateTime(_row); }
		std::optional<DBTimePoint> GetTimePointOpt( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->DateTimeOpt(_row); }
		uint GetUInt( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->UInt(_row); }
		std::optional<uint> GetUIntOpt( uint position )Ι{ ASSERT( position<_bindings.size() ); return _bindings[position]->UIntOpt(_row); }
		α Size()Ι->uint { return _bindings.size(); }
	private:
		const vector<up<IBindings>>& _bindings;
		mutable uint _row{0};
		mutable uint _index{};
	};
}