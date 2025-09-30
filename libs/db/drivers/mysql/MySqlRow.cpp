#include "MySqlRow.h"
#include <jde/fwk/utils/mathUtils.h>
#include <jde/db/Value.h>
#include <jde/db/DBException.h>
#include <jde/db/generators/Functions.h>
#define let const auto

namespace Jde::DB{
	using namespace std::chrono;

	Ω toValue( const boost::mysql::field_view& field, SL sl )ε->Value{
		Value v;
		switch( field.kind() ){
			using enum boost::mysql::field_kind;
    	case null: break;
			case int64: v = Value{ field.as_int64() }; break;
			case uint64: v = Value{ field.as_uint64() }; break;
			case string: v = Value{ field.as_string() }; break;
			case float_: v = Value{ field.as_float() }; break;
			case double_: v = Value{ field.as_double() }; break;
			case date:{
				std::chrono::time_point<std::chrono::system_clock, std::chrono::days> tp{ field.as_date().get_time_point() };
				DBTimePoint time{ duration_cast<DBTimePoint::duration>( tp.time_since_epoch() ) };
				v = Value{ time };
			}break;
			case datetime:{
				boost::mysql::datetime::time_point dt{ field.as_datetime().get_time_point() };
				v = Value{ DBTimePoint{duration_cast<DBTimePoint::duration>(dt.time_since_epoch())} };
			}break;
			case time:{
				std::chrono::microseconds micros{ field.as_time() };
				v = Value{ micros.count() };
			}break;
			case blob:
				v = Value{ vector<uint8_t>{field.as_blob().begin(), field.as_blob().end()} }; break;
		}
		return v;
	}

	α MySql::ToRow( const mysql::row_view& mySqlRow, SL sl )->Row{
		vector<Value> values;
		for( auto&& field : mySqlRow )
			values.push_back( toValue(field, sl) );
		return Row{ move(values) };
	}
}