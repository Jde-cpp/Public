#include "field.h"

namespace Jde::DB{
	α MySql::ToField( const Value& v, SL sl )ε->mysql::field_view{
		switch( v.Type() ){
			using enum EValue;
			case Null: return mysql::field_view{};
			case String: return mysql::field_view( v.get_string() );
			case Bool: return mysql::field_view( v.get_bool() );
			case Int8: return mysql::field_view( v.get_int8() );
			case Int32: return mysql::field_view( v.get_int32() );
			case Int64: return mysql::field_view( v.get_int() );
			case UInt32: return mysql::field_view( v.get_uint32() );
			case UInt64: return mysql::field_view( v.get_uint() );
			case Double: return mysql::field_view( v.get_double() );
			case Time: {
				using namespace std::chrono;
				DBTimePoint time = v.get_time();//duration<_GLIBCXX_CHRONO_INT64_T, nano>
				time_point<system_clock, duration<std::int64_t, std::micro>>
					mysqlTime{ duration_cast<duration<std::int64_t, std::micro>>(time.time_since_epoch()) };
				mysql::datetime dt{ mysqlTime };
				return mysql::field_view{ dt };
			}
			case Bytes: {
				auto& bytes = v.get_bytes();
				return bytes.size()==0 ? mysql::field_view{} : mysql::field_view{ mysql::blob_view{bytes.data(), bytes.size()} };
			}
			default:
				throw Exception{ sl, ELogLevel::Error, "{} dataValue not implemented", v.TypeName() };
		}
	}
}