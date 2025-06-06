#pragma once
#include <jde/db/IRow.h>
#include "Binding.h"
#include "Bindings.h"
//#include <jde/Assert.h>

namespace Jde::DB::Odbc{
	struct OdbcRow : public IRow{
		OdbcRow( const vector<up<Binding>>& bindings )ι;
		α Reset()Ι->void{_index=0;}
		α ToRow()ι->Row;
		α operator[]( uint value )Ι->const Value& override;
		α operator[]( uint i )ι->Value& override;
		α GetBit( uint position )Ι->bool override;
		α MoveString( uint i )ε->string override;
		α GetString( uint position )Ι->string override;
		int64_t GetInt( uint position )Ι override;
		int32_t GetInt32( uint position )Ι override;
		std::optional<_int> GetIntOpt( uint position )Ι override;
		double GetDouble( uint position )Ι override;
		float GetFloat( uint position )Ι{ return static_cast<float>( GetDouble(position) ); }
		std::optional<double> GetDoubleOpt( uint position )Ι override;
		DBTimePoint GetTimePoint( uint position )Ι override;
		std::optional<DBTimePoint> GetTimePointOpt( uint position )Ι override;
		uint GetUInt( uint position )Ι override;
		α GetUIntOpt( uint position )Ι->std::optional<uint> override;
		α Size()Ι->uint override { return _bindings.size(); }
	private:
		const vector<up<Binding>>& _bindings;
		vector<optional<Value>> _values;
	};

	struct OdbcRowMulti : public IRow{
		OdbcRowMulti( const vector<up<IBindings>>& bindings )ι:_bindings{bindings}{};
		void Reset()Ι{ _index=0; ++_row; }
		void ResetRowIndex()Ι{ _row=0; }

		α operator[]( uint position )Ι->const Value& override{ ASSERT( position<_bindings.size() );  return _bindings[position]->Object(_row); }
		α operator[]( uint i )ι->Value& override{ ASSERT( i<_bindings.size() );  return _bindings[i]->Object(_row); }
		α GetBit( uint position )Ι->bool override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Bit(_row); }
		α GetString( uint position )Ι->string override{ ASSERT( position<_bindings.size() ); return _bindings[position]->ToString(_row); }
		α MoveString( uint i )ε->string override;
		int64_t GetInt( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Int(_row); }
		int32_t GetInt32( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Int32(_row); }
		std::optional<_int> GetIntOpt( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->IntOpt(_row); }
		double GetDouble( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Double(_row); }
		float GetFloat( uint position )Ι{ return static_cast<float>( GetDouble(position) ); }
		std::optional<double> GetDoubleOpt( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->DoubleOpt(_row); }
		DBTimePoint GetTimePoint( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->DateTime(_row); }
		std::optional<DBTimePoint> GetTimePointOpt( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->DateTimeOpt(_row); }
		uint GetUInt( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->UInt(_row); }
		std::optional<uint> GetUIntOpt( uint position )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->UIntOpt(_row); }
		α Size()Ι->uint override { return _bindings.size(); }
	private:
		const vector<up<IBindings>>& _bindings;
		mutable uint _row{0};
	};
}