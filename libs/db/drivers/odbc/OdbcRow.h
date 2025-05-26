#pragma once
#include <jde/db/IRow.h>
#include "Binding.h"
#include "Bindings.h"
//#include <jde/Assert.h>

namespace Jde::DB::Odbc{
	struct OdbcRow : public IRow{
		OdbcRow( const vector<up<Binding>>& bindings )ι;
		α Reset()Ι->void{_index=0;}
		α Move()ι->up<IRow> override;
		α operator[]( uint value )Ι->const Value& override;
		α operator[]( uint i )ι->Value& override;
		α GetBit( uint position, SRCE )Ι->bool override;
		α MoveString( uint i, SRCE )ε->string override;
		α GetString( uint position, SRCE )Ι->string override;
		int64_t GetInt( uint position, SRCE )Ι override;
		int32_t GetInt32( uint position, SRCE )Ι override;
		std::optional<_int> GetIntOpt( uint position, SRCE )Ι override;
		double GetDouble( uint position, SRCE )Ι override;
		float GetFloat( uint position )Ι{ return static_cast<float>( GetDouble(position) ); }
		std::optional<double> GetDoubleOpt( uint position, SRCE )Ι override;
		DBTimePoint GetTimePoint( uint position, SRCE )Ι override;
		std::optional<DBTimePoint> GetTimePointOpt( uint position, SRCE )Ι override;
		uint GetUInt( uint position, SRCE )Ι override;
		α GetUIntOpt( uint position, SRCE )Ι->std::optional<uint> override;
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
		α Move()ι->up<IRow> override;
		α GetBit( uint position, SL )Ι->bool override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Bit(_row); }
		α GetString( uint position, SL )Ι->string override{ ASSERT( position<_bindings.size() ); return _bindings[position]->ToString(_row); }
		α MoveString( uint i, SRCE )ε->string override;
		int64_t GetInt( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Int(_row); }
		int32_t GetInt32( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Int32(_row); }
		std::optional<_int> GetIntOpt( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->IntOpt(_row); }
		double GetDouble( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->Double(_row); }
		float GetFloat( uint position )Ι{ return static_cast<float>( GetDouble(position, SRCE_CUR) ); }
		std::optional<double> GetDoubleOpt( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->DoubleOpt(_row); }
		DBTimePoint GetTimePoint( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->DateTime(_row); }
		std::optional<DBTimePoint> GetTimePointOpt( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->DateTimeOpt(_row); }
		uint GetUInt( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->UInt(_row); }
		std::optional<uint> GetUIntOpt( uint position, SL )Ι override{ ASSERT( position<_bindings.size() ); return _bindings[position]->UIntOpt(_row); }
		α Size()Ι->uint override { return _bindings.size(); }
	private:
		const vector<up<IBindings>>& _bindings;
		mutable uint _row{0};
	};
}