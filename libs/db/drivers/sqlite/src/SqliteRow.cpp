#include <sqlite3.h>
#include "SqliteRow.h"
#include <jde/db/DBException.h>

#define let const auto

namespace Jde::DB::Sqlite{
	α Bind( sqlite3_stmt& stmt, const vector<Value>& params, SL sl )ε->void{
		for( uint i=0; i<params.size(); ++i ){
			let& v = params[i];
			let col = (int)i+1;
			int rc{ SQLITE_OK };
			switch( v.Type() ){
				using enum EValue;
				case Null: rc = sqlite3_bind_null( &stmt, col ); break;
				case String: rc = sqlite3_bind_text( &stmt, col, v.get_string().data(), (int)v.get_string().size(), SQLITE_TRANSIENT ); break;
				case Bool: rc = sqlite3_bind_int( &stmt, col, v.get_bool() ? 1 : 0 ); break;
				case Int8: case Int32: case Int64: case UInt32: case UInt64:
					rc = sqlite3_bind_int64( &stmt, col, v.get_number<int64_t>() ); break;
				case Double: rc = sqlite3_bind_double( &stmt, col, v.get_double() ); break;
				case Time: rc = sqlite3_bind_int64( &stmt, col, duration_cast<std::chrono::seconds>(v.get_time()-Chrono::Epoch()).count() ); break; //unix epoch seconds - must match SqliteSyntax::UtcNow.
				case Bytes:{
					let& bytes = v.get_bytes();
					rc = sqlite3_bind_blob( &stmt, col, bytes.data(), (int)bytes.size(), SQLITE_TRANSIENT ); break;
				}
			}
			THROW_IFSL( rc!=SQLITE_OK, "bind param[{}] failed: rc={}", i, rc );
		}
	}

	α ToValue( sqlite3_stmt& stmt, int col )ε->Value{
		switch( sqlite3_column_type(&stmt, col) ){
			case SQLITE_NULL: return Value{};
			case SQLITE_INTEGER:{
				let v = sqlite3_column_int64( &stmt, col );
				//sqlite stores datetimes as epoch ints & bits as 0/1 - the declared type recovers them. Expressions have no decltype and stay ints.
				if( let declared = sqlite3_column_decltype(&stmt, col); declared ){
					let lower = Str::ToLower( declared );
					if( lower.find("date")!=string::npos || lower.find("time")!=string::npos )
						return Value{ DBTimePoint{Chrono::Epoch()+std::chrono::seconds{v}} };
					if( lower=="bit" || lower=="bool" || lower=="boolean" )
						return Value{ v!=0 };
				}
				return Value{ v };
			}
			case SQLITE_FLOAT: return Value{ sqlite3_column_double(&stmt, col) };
			case SQLITE_BLOB:{
				let p = (const uint8_t*)sqlite3_column_blob( &stmt, col );
				return Value{ vector<uint8_t>{p, p+sqlite3_column_bytes(&stmt, col)} };
			}
			default: return Value{ string{(const char*)sqlite3_column_text(&stmt, col), (uint)sqlite3_column_bytes(&stmt, col)} };
		}
	}

	α ToRow( sqlite3_stmt& stmt )ε->Row{
		let count = sqlite3_column_count( &stmt );
		vector<Value> values; values.reserve( count );
		for( int i=0; i<count; ++i )
			values.push_back( ToValue(stmt, i) );
		return Row{ move(values) };
	}
}
