#pragma once
#include <jde/db/IDataSource.h>
#include <jde/db/IRow.h>

namespace Jde::DB::MySql{
	struct MySqlRow : public IRow{
		MySqlRow( const mysqlx::Row& row, SRCE )ε;
		MySqlRow( MySqlRow&& rhs )ι:_values{move(rhs._values)}{};
		virtual ~MySqlRow(){}
		α Move()ι->up<IRow> override;
		α operator[]( uint position )Ε->Value override;
		α GetString( uint position, SRCE )Ε->std::string override;
		α GetBit( uint position, SRCE )Ε->bool override;
		α GetInt( uint position, SRCE )Ε->_int override;
		α GetInt32( uint position, SRCE )Ε->int32_t override{ return static_cast<int32_t>( GetInt(position) ); }
		α GetIntOpt( uint position, SRCE )Ε->optional<_int> override;
		α GetUInt( uint position, SRCE )Ε->uint override;
		α GetUIntOpt( uint position, SRCE )Ε->optional<uint> override;

		α GetDouble( uint position, SRCE )Ε->double override;
		α GetDoubleOpt( uint position, SRCE )Ε->std::optional<double> override;
		α GetTimePoint( uint position, SRCE )Ε->DBTimePoint override;
		α GetTimePointOpt( uint position, SRCE )Ε->std::optional<DBTimePoint> override;
		α Size()Ι->uint override{ return _values.size(); }
	private:
		vector<Value> _values;
	};
}