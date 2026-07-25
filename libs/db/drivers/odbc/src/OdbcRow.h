#pragma once
#include <jde/db/Row.h>
#include "Binding.h"

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

}