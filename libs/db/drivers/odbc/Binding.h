#pragma once
#include <jde/db/DBException.h>

#define let const auto
namespace Jde::DB::Odbc{
#pragma warning( push )
#pragma warning (disable: 4716)
	struct Binding{
		Binding( SQLSMALLINT type, SQLSMALLINT cType, SQLLEN output=0 )ι:
			_dbType{ type },
			CodeType{cType},
			Output{output}
		{}
		virtual ~Binding()=default;
		Ω GetBinding( SQLSMALLINT type )ε->up<Binding>;
		Ω Create( Value parameter )ι->up<Binding>;

		β Data()ι->void* = 0;
		β GetValue()Ι->Value = 0;
#define $ [[noreturn]] β
		$ GetBit()Ι->bool{ THROW( "{} not implemented for DBType={} CodeType={}", "bit", _dbType, CodeType ); }
		$ to_string()Ε->string{ THROW( "to_string not implemented for DBType='{}' CodeType='{}' {}", _dbType, CodeType, "GetTypeName<decltype(this)>()" ); }
		$ GetInt()Ε->int64_t{ THROW( "{} not implemented for DBType={} CodeType={}", "GetInt", _dbType, CodeType ); }
		$ GetInt32()Ε->int32_t{ THROW( "{} not implemented for DBType={} CodeType={}", "GetInt32", _dbType, CodeType ); }
		$ GetIntOpt()Ε->std::optional<_int>{ THROW( "{} not implemented for DBType={} CodeType={} {}", "GetIntOpt", _dbType, CodeType, "GetTypeName<decltype(this)>()" ); }
		$ GetDouble()Ε->double{ THROW( "{} not implemented for DBType={} CodeType={}", "GetDouble", _dbType, CodeType ); }
		β GetFloat()Ε->float{ return static_cast<float>( GetDouble() ); }
		$ GetDoubleOpt()Ε->std::optional<double>{ THROW( "{} not implemented for DBType={} CodeType={}", "GetDoubleOpt", _dbType, CodeType ); }
		$ GetDateTime()Ε->DBTimePoint{ THROW( "{} not implemented for DBType={} CodeType={}", "GetDateTime", _dbType, CodeType ); }
		$ GetDateTimeOpt()Ε->std::optional<DBTimePoint>{ THROW( "{} not implemented for DBType={} CodeType={}", "GetDateTimeOpt", _dbType, CodeType); }
		$ GetUInt()Ε->uint{ THROW( "{} not implemented for DBType={} CodeType={}", "GetUInt", _dbType, CodeType); }
		β GetUInt32(uint)Ε->uint32_t{ return static_cast<uint32_t>(GetUInt()); }
		$ GetUIntOpt()Ε->std::optional<uint>{ THROW( "{} not implemented for DBType={} CodeType={} - {}", "GetUIntOpt", _dbType, CodeType, "GetTypeName<decltype(this)>()" ); };
		α IsNull()Ι->bool{ return GetOutput()==SQL_NULL_DATA; }
		β Size()Ι->SQLULEN{ return 0; }
		β DecimalDigits()Ι->SQLSMALLINT{return 0;}
		β BufferLength()Ι->SQLLEN{return 0;}
		β DBType()Ι->SQLSMALLINT{ return _dbType; }
		SQLLEN Output; //Binding fills in for each row.
		β GetOutput()Ι->SQLLEN{ return Output; }
		SQLSMALLINT CodeType{0};
	private:
		SQLSMALLINT _dbType;
	};
#undef $
#pragma warning( pop )
	template <class T, SQLSMALLINT TSql, SQLSMALLINT TC>
	struct TBinding : Binding{
		TBinding()ι:Binding{TSql,TC}{}
		α Data()ι->void* override{ return &_data; }
	protected:
		SQL_NUMERIC_STRUCT _data;
	};

	struct BindingNull final : Binding{
		BindingNull( SQLSMALLINT type=SQL_VARCHAR )ι:
			Binding{ type, SQL_C_CHAR, SQL_NULL_DATA }
		{}
		α Data()ι->void* override{ return nullptr; }
		α GetValue()Ι->Value override{ return Value{}; }
	};

	struct BindingString final: Binding{
		BindingString( SQLSMALLINT type, SQLLEN size )ι:
			Binding{ type, SQL_C_CHAR, size },
			_buffer{std::make_unique_for_overwrite<char[]>( size )}{
		}
		BindingString( sv value )ι:
			Binding{ SQL_VARCHAR, SQL_C_CHAR, (SQLLEN)value.size() },
			_buffer{ std::make_unique_for_overwrite<char[]>(value.size()) }{
			std::copy(value.begin(), value.end(), _buffer.get());
		}
		α Data()ι->void* override{ return _buffer.get(); }
		α GetValue()Ι->DB::Value override{ return IsNull() ? Value{} : Value{to_string()}; }
		α to_string()Ι->string override{ return GetOutput()==-1 || !_buffer ? string{} : string{ _buffer.get(), _buffer.get()+GetOutput() }; }
		α BufferLength()Ι->SQLLEN override{ return std::max<SQLLEN>( GetOutput(), 0 ); }
		α Size()Ι->SQLULEN override{ return (SQLULEN)BufferLength(); }
	private:
		up<char[]> _buffer;
	};

	struct BindingBinary final :Binding{
		BindingBinary():Binding{ SQL_VARBINARY, SQL_C_BINARY }{}
		BindingBinary( SQLLEN size ):Binding{ SQL_VARBINARY, SQL_C_BINARY, size==0 ? SQL_NULL_DATA : size },_value{ vector<uint8_t>(size) }{}
		BindingBinary( Value v ):
			Binding{ SQL_VARBINARY, SQL_C_BINARY, v.get_bytes().size()==0 ? SQL_NULL_DATA : (SQLLEN)v.get_bytes().size()  },
			_value{ move(v) }
		{}
		α Data()ι->void* override{ return IsNull() ? nullptr : GetValue().get_bytes().data(); }
		α GetValue()Ι->Value override{ return _value; }
		α GetOutput()Ι->SQLLEN override{ return Size()==0 ? SQL_NULL_DATA : Binding::GetOutput(); }

		α Size()Ι->SQLULEN override { return _value.get_bytes().size(); }
		Value _value;
	};
	struct BindingGuid final : Binding{
		BindingGuid():Binding{ SQL_GUID, SQL_C_GUID }{}
		BindingGuid( const uuid& v ):
			Binding{ SQL_GUID, SQL_C_GUID, 16 }{
			memcpy( &_guid, v.data(), sizeof(SQLGUID) );
		}
		α Data()ι->void* override{ return &_guid; }
		α GetValue()Ι->Value override { uuid id; memcpy( id.data(), &_guid, sizeof(SQLGUID) ); return Value{ id }; }
		α GetOutput()Ι->SQLLEN override{ return sizeof(SQLGUID); }
		α Size()Ι->SQLULEN override { return sizeof(SQLGUID); }

		SQLGUID _guid;
	};
	struct BindingBit final : Binding{
		BindingBit()ι:BindingBit{ (SQLSMALLINT)SQL_BIT }{}//-7
		BindingBit( SQLSMALLINT type )ι:Binding{ type, SQL_C_BIT }{}
		BindingBit( bool value )ι: Binding{ SQL_CHAR, SQL_C_BIT },_data{value ? '\1' : '\0'}{}
		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data=='\1'}; }
		α GetBit()Ι->bool override{ return _data!=0; }
		α Size()Ι->SQLULEN{return 1;}
		α GetInt()Ι->int64_t override{ return static_cast<int64_t>(_data); }
		α to_string()Ι->string override{ return _data ? "true" : "false"; }
	private:
		char _data;
	};
	struct BindingInt8 final : Binding{
		BindingInt8( SQLSMALLINT type=SQL_TINYINT )ι: Binding{ type, SQL_C_TINYINT }{}
		BindingInt8( int8_t value )ι: Binding{ SQL_TINYINT, SQL_C_TINYINT },_data{value}{}
		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }
		α GetInt32()Ι->int32_t override{ return _data; }

		int64_t GetInt()Ι override{ return GetInt32(); }
		uint GetUInt()Ι override{ return static_cast<uint>(GetInt32()); }
		std::optional<_int> GetIntOpt()Ι{ std::optional<_int> value; if( !IsNull() )value=GetInt(); return value; }
		std::optional<uint> GetUIntOpt()Ι override{ std::optional<uint> optional; if( !IsNull() ) optional=GetUInt(); return optional; };
	private:
		int8_t _data;
	};

	struct BindingInt32 final : Binding{
		BindingInt32( SQLSMALLINT type=SQL_INTEGER )ι: Binding{ type, SQL_C_SLONG }{}
		BindingInt32( int value )ι: Binding{ SQL_INTEGER, SQL_C_SLONG },_data{value}{}
		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }
		α GetInt32()Ι->int32_t override{ return _data; }
		int64_t GetInt()Ι override{ return GetInt32(); }
		uint GetUInt()Ι override{ return static_cast<uint>(GetInt32()); }
		std::optional<_int> GetIntOpt()Ι{ std::optional<_int> value; if( !IsNull() )value=GetInt(); return value; }
		std::optional<uint> GetUIntOpt()Ι override{ std::optional<uint> optional; if( !IsNull() ) optional=GetUInt(); return optional; };
	private:
		int _data;
	};

	struct BindingDecimal final : Binding
	{};

	struct BindingInt final : Binding{
		BindingInt( SQLSMALLINT type=SQL_C_SBIGINT )ι: Binding{ type, SQL_C_SBIGINT }{}
		BindingInt( _int value )ι: Binding{ SQL_BIGINT, SQL_C_SBIGINT },_data{value}{}
		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }
		int64_t GetInt()Ι override{ return _data; }
		uint GetUInt()Ι override{ return static_cast<uint>( GetInt() ); }
		std::optional<uint> GetUIntOpt()Ι{ std::optional<uint> value; if(!IsNull())value=GetUInt(); return value; }
		β GetDateTime()Ι->DBTimePoint{ return Clock::from_time_t(_data); }
	private:
		_int _data ;
	};

	struct BindingTimeStamp final:  Binding{
		BindingTimeStamp( SQLSMALLINT type=SQL_C_TYPE_TIMESTAMP )ι: Binding{ type, SQL_C_TYPE_TIMESTAMP }{}
		BindingTimeStamp( SQL_TIMESTAMP_STRUCT value )ι: Binding{ SQL_TYPE_TIMESTAMP, SQL_C_TYPE_TIMESTAMP },_data{value}{}
		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{GetDateTime()}; }
		DBTimePoint GetDateTime()Ι{ return IsNull() ? DBTimePoint{} : Chrono::ToTimePoint( _data.year, (uint8)_data.month, (uint8)_data.day, (uint8)_data.hour, (uint8)_data.minute, (uint8)_data.second, Duration(_data.fraction) ); }
		std::optional<DBTimePoint> GetDateTimeOpt()Ι override{ return IsNull() ? std::nullopt : std::make_optional(GetDateTime()); }
	private:
		SQL_TIMESTAMP_STRUCT _data;
	};

	struct BindingUInt final : Binding{
		BindingUInt( SQLSMALLINT type )ι:	Binding{ type, SQL_C_UBIGINT }{}
		BindingUInt( uint value )ι: Binding{ SQL_INTEGER, SQL_C_SLONG }, _data{value}{}
		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }
		uint GetUInt()Ι override{ return _data; }
	private:
		uint _data ;
	};

	struct BindingDateTime final : Binding{
		BindingDateTime( SQLSMALLINT type=SQL_TYPE_TIMESTAMP )ι:Binding{ type, SQL_C_TIMESTAMP, sizeof(SQL_TIMESTAMP_STRUCT) }{}

		BindingDateTime( const optional<DBTimePoint>& value )ι;

		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override { return IsNull() ? Value{} : Value{ GetDateTime() }; }
		SQLLEN BufferLength()Ι override{ return sizeof(_data); }
		//https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/column-size?view=sql-server-ver16&redirectedfrom=MSDN
		//https://wezfurlong.org/blog/2005/Nov/calling-sqlbindparameter-to-bind-sql-timestamp-struct-as-sql-c-type-timestamp-avoiding-a-datetime-overflow/
		SQLULEN Size()Ι override{ return 23; }//23 works with 0
		SQLSMALLINT DecimalDigits()Ι{ return 3; }//https://stackoverflow.com/questions/40918607/cannot-bind-a-sql-type-timestamp-value-using-odbc-with-ms-sql-server-hy104-inv
		DBTimePoint GetDateTime()Ι override{ return IsNull() ? DBTimePoint() : Chrono::ToTimePoint( _data.year, (uint8)_data.month, (uint8)_data.day, (uint8)_data.hour, (uint8)_data.minute, (uint8)_data.second, Duration(_data.fraction) );}
		std::optional<DBTimePoint> GetDateTimeOpt()Ι override{
			std::optional<DBTimePoint> value;
			if( !IsNull() )
				value = GetDateTime();
			return value;
		}
		SQL_TIMESTAMP_STRUCT _data;
	};

	struct BindingDouble final : Binding{
		BindingDouble( SQLSMALLINT type=SQL_DOUBLE )ι:	Binding{ type, SQL_C_DOUBLE }{}
		BindingDouble( double value )ι: Binding{ SQL_DOUBLE, SQL_C_DOUBLE },_data{value}{}
		BindingDouble( const optional<double>& value )ι:
			Binding{ SQL_DOUBLE, SQL_C_DOUBLE },_data{value.has_value() ? value.value() : 0.0}{
			if( !value.has_value() )
				Output = SQL_NULL_DATA;
		}

		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }

		α GetDouble()Ι->double override{ return _data; }
		std::optional<double> GetDoubleOpt()Ι{ std::optional<double> value; if( !IsNull() ) value = GetDouble();	return value; }
	private:
		double _data;
	};
	struct BindingNumeric final : TBinding<SQL_NUMERIC_STRUCT,SQL_NUMERIC,SQL_C_NUMERIC>{
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{GetDouble()}; }
		α GetDouble()Ι->double override{//https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/retrieve-numeric-data-sql-numeric-struct-kb222831?view=sql-server-ver15
			uint divisor = (uint)std::pow( 1, _data.scale );
			_int value = 0, last=1;
			for( uint i=0; i<SQL_MAX_NUMERIC_LEN; ++i ){
				const int current = _data.val[i];
				const int a = current % 16;
				const int b = current / 16;
				value += last * a;
				last *= 16;
	         value += last * b;
				last *= 16;
			}
			return (_data.sign ? 1 : -1)*(double)value/divisor;
		}
		α GetDoubleOpt()Ι->std::optional<double> override{ std::optional<double> value; if( !IsNull() ) value = GetDouble(); return value; }
		α GetInt()Ι->_int override{ return (_int)GetDouble(); }
		α GetUInt()Ι->uint override{ return (uint)GetDouble(); }
	};

	struct BindingFloat final : Binding{
		BindingFloat( SQLSMALLINT type=SQL_FLOAT )ι:	Binding{ type, SQL_C_FLOAT }{}
		BindingFloat( float value )ι: Binding{ SQL_FLOAT, SQL_C_FLOAT },_data{value}{}

		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }

		double GetDouble()Ι override{ return _data; }
		std::optional<double> GetDoubleOpt()Ι{ std::optional<double> value; if( !IsNull() ) value = GetDouble();	return value; }
	private:
		float _data;
	};

	struct BindingInt16 final : Binding{
		BindingInt16()ι:Binding{ SQL_SMALLINT, SQL_C_SSHORT }{}
		BindingInt16( int16_t value )ι: Binding{ SQL_SMALLINT, SQL_C_SSHORT },_data{value}{}

		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override { return IsNull() ? Value{} : Value{_data}; }
		α GetUInt()Ι->uint override{ return static_cast<uint>(_data); }
		α GetInt()Ι->_int override{ return static_cast<_int>(_data); }
		α GetIntOpt()Ι->optional<_int> override{ std::optional<_int> value; if( !IsNull() ) value = GetInt(); return value; }
		α GetDouble()Ι->double override{ return _data; }
		α GetDoubleOpt()Ι->std::optional<double>{ std::optional<double> value; if( !IsNull() ) value = GetDouble(); return value; }
	private:
		int16_t _data;
	};

	struct BindingUInt8 final : Binding{
		BindingUInt8()ι:Binding{ SQL_TINYINT, SQL_C_TINYINT }{}
		BindingUInt8( uint8_t value )ι: Binding{ SQL_TINYINT, SQL_C_TINYINT },_data{value}{}

		α Data()ι->void* override{ return &_data; }
		α GetValue()Ι->Value override{ return IsNull() ? Value{} : Value{_data}; }

		α GetUInt()Ι->uint override{ return static_cast<uint>(_data); }
		α GetUIntOpt()Ι->std::optional<uint>{ std::optional<_int> value; if( !IsNull() ) value = GetUInt(); return value; }
		α GetInt32()Ι->int32_t override{ return static_cast<int32_t>(_data); }
		α GetIntOpt()Ι->std::optional<_int> override{ std::optional<_int> value; if( !IsNull() ) value = GetInt(); return value; }
		α GetInt()Ι->_int override{ return static_cast<_int>(_data); }
		α GetDouble()Ι->double override{ return _data; }
		α GetDoubleOpt()Ι->std::optional<double>{ std::optional<double> value; if( !IsNull() ) value = GetDouble();	return value; }
	private:
		uint8_t _data;
	};

	Ξ Binding::GetBinding( SQLSMALLINT type )ε->up<Binding>{
		up<Binding> pBinding;
		switch( type ){
			case SQL_BIT: pBinding = mu<BindingBit>(); break;
			case SQL_TINYINT: pBinding = mu<BindingUInt8>(); break;
			case SQL_INTEGER: pBinding = mu<BindingInt32>( type ); break;
			case SQL_DECIMAL:  pBinding = mu<BindingDouble>( type ); break;
			case SQL_SMALLINT: pBinding = mu<BindingInt16>(); break;
			case SQL_FLOAT: pBinding = mu<BindingFloat>( type ); break;
			case SQL_REAL: pBinding = mu<BindingDouble>( type ); break;
			case SQL_DOUBLE: pBinding = mu<BindingDouble>( type ); break;
			case SQL_DATETIME: pBinding = mu<BindingDateTime>( type ); break;
			case SQL_BIGINT: pBinding = mu<BindingInt>( type ); break;
			case SQL_TYPE_TIMESTAMP: pBinding = mu<BindingTimeStamp>( type ); break;
			case SQL_NUMERIC:	pBinding = mu<BindingNumeric>(); break;
			case SQL_VARBINARY:{ pBinding = mu<BindingBinary>(); break;}
			case SQL_GUID: { pBinding = mu<BindingGuid>(); break; }
			default: THROW( "Binding type '{}' is not implemented.", type );
		}
		return pBinding;
	}
	using std::get;
	Ξ Binding::Create( Value param )ι->up<Binding>{
		up<Binding> pBinding;
		switch( param.Type() ){
		using enum EValue;
		case Null: pBinding = mu<BindingNull>(); break;
		case String: pBinding = mu<BindingString>( param.get_string() ); break;
		case Bool: pBinding = mu<BindingBit>( param.get_bool() ); break;
		case Bytes: pBinding = mu<BindingBinary>( move( param ) ); break;
		case Int8: pBinding = mu<BindingInt8>( param.get_int8() ); break;
		case Int32:
			pBinding = mu<BindingInt32>( param.get_int32() );
		break;
		case Int64:
			pBinding = mu<BindingInt>( param.get_int() );
		break;
		case UInt32:
			pBinding = mu<BindingInt32>( (int)param.get_uint32() );
		break;
		case UInt64:
			pBinding = mu<BindingInt>( (_int)param.get_uint() );
		break;
		case Double:
			pBinding = mu<BindingDouble>( param.get_double() );
		break;
		case Time:
			pBinding = mu<BindingDateTime>( param.get_time() );
		break;
		default:
			ASSERT( false );
		}
		return pBinding;
	}
	inline BindingDateTime::BindingDateTime( const optional<DBTimePoint>& tp )ι:
		BindingDateTime{}{
		if( !tp.has_value() )
			Output = SQL_NULL_DATA;
		else{
			using namespace std::chrono;
			let date{ floor<days>(*tp) };
			const year_month_day ymd{ date };
			const hh_mm_ss time{ *tp-date };

			_data.year = (SQLSMALLINT)(int)ymd.year();
			_data.month = (SQLUSMALLINT)(unsigned)ymd.month();
			_data.day = (SQLUSMALLINT)(unsigned)ymd.day();
			_data.hour = (SQLUSMALLINT)(unsigned)time.hours().count();
			_data.minute = (SQLUSMALLINT)time.minutes().count();
			_data.second = (SQLUSMALLINT)time.seconds().count();
			_data.fraction = time.subseconds().count();
		}
	}
}
#undef let