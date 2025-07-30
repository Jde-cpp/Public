#pragma once
#include <jde/db/Value.h>
//#include <jde/db/DBException.h>

namespace Jde::DB::Odbc{
#pragma warning( push )
#pragma warning (disable: 4716)
#define $ [[noreturn]] β
	struct IBindings{
		IBindings( uint rowCount, SQLULEN size=0 ): _output{ new SQLLEN[rowCount] }, RowCount{ rowCount }, _size{ size }{}
		virtual ~IBindings()=0;
		Ω Create( SQLSMALLINT type, uint rowCount )->up<IBindings>;
		Ω Create( SQLSMALLINT type, uint rowCount, uint size )ε->up<IBindings>;
		//β Move()ι->IRow_bindings = 0;
		β Data()ι->void* = 0;
		β Object( uint i )Ι->Value=0;
		β Object( uint )ι->Value&{ THROW( "{} not implemented for DBType={} CodeType={}", "bit", DBType(), CodeType() ); }
		β CodeType()Ι->SQLSMALLINT = 0;
		β DBType()Ι->SQLSMALLINT = 0;

		$ Bit( uint )Ι->bool{ THROW( "{} not implemented for DBType={} CodeType={}", "bit", DBType(), CodeType() ); }
		$ ToString( uint )Ι->string{ THROW( "ToString not implemented for DBType='{}' CodeType='{}' {}", DBType(), CodeType(), "GetTypeName<decltype(this)>()" ); }
		$ MoveString( uint )ι->string{ THROW( "MoveString not implemented for DBType='{}' CodeType='{}' {}", DBType(), CodeType(), "GetTypeName<decltype(this)>()" ); }
		$ Int( uint )Ι->int64_t{ THROW( "{} not implemented for DBType={} CodeType={}", "GetInt", DBType(), CodeType() ); }
		$ Int32( uint )Ι->int32_t{ THROW( "{} not implemented for DBType={} CodeType={}", "Int32", DBType(), CodeType() ); }
		$ IntOpt( uint )Ι->optional<_int>{ THROW( "{} not implemented for DBType={} CodeType={} {}", "IntOpt", DBType(), CodeType(), "GetTypeName<decltype(this)>()" ); }
		$ Double( uint )Ι->double{ THROW( "{} not implemented for DBType={} CodeType={}", "Double", DBType(), CodeType() ); }
		β GetFloat( uint i )Ι->float{ return static_cast<float>( Double(i) ); }
		$ DoubleOpt( uint )Ι->optional<double>{ THROW( "{} not implemented for DBType={} CodeType={}", "DoubleOpt", DBType(), CodeType() ); }
		$ DateTime( uint )Ι->DBTimePoint{ THROW( "{} not implemented for DBType={} CodeType={}", "DateTime", DBType(), CodeType() ); }
		$ DateTimeOpt( uint )Ι->optional<DBTimePoint>{ THROW( "{} not implemented for DBType={} CodeType={}", "DateTimeOpt", DBType(), CodeType()); }
		$ UInt( uint )Ι->uint{ THROW( "{} not implemented for DBType={} CodeType={}", "UInt", DBType(), CodeType()); }
		//β GetUInt32(uint position )Ι->uint32_t{ return static_cast<uint32_t>(UInt()); }
		$ UIntOpt( uint )Ι->optional<uint>{ THROW( "{} not implemented for DBType={} CodeType={} - {}", "UIntOpt", DBType(), CodeType(), "GetTypeName<decltype(this)>()" ); };
		α IsNull( uint i )Ι->bool{ return _output[i]==SQL_NULL_DATA; }
		α Output( uint i )Ι->SQLLEN{ return _output[i]; }
		α OutputPtr()ι->SQLLEN*{ return _output.get(); }
		β Size()Ι->SQLULEN{return _size;}
		β DecimalDigits()Ι->SQLSMALLINT{return 0;}
		β BufferLength()Ι->SQLLEN{return 0;}
		
	private:
		up<SQLLEN[]> _output;
		uint RowCount;
		SQLULEN _size;
	};
#undef $
	inline IBindings::~IBindings(){};
#pragma warning( pop )
	template <class T, SQLSMALLINT TSql, SQLSMALLINT TC>
	struct TBindings : IBindings
	{
		TBindings( uint rowCount, SQLULEN size ):IBindings{ rowCount, size }, _pBuffer{ new T[rowCount*size]() }{}
		TBindings( uint rowCount )ι:TBindings{ rowCount, 1 }{}
		void* Data()ι override{ return _pBuffer.get(); }
		static consteval α SqlType()->SQLSMALLINT{ return TSql; }
		β CodeType()Ι->SQLSMALLINT override{ return TC; }
		β DBType()Ι->SQLSMALLINT override{ return SqlType(); }
	protected:
		up<T[]> _pBuffer;
	};

	#define base TBindings<char, TSql, SQL_C_CHAR>
	template<SQLSMALLINT TSql>
	struct BindingStrings final : base{
		BindingStrings( uint rowCount, SQLULEN size ):base{ rowCount, size }{}
		α Object( uint i )Ι->Value override{ return base::IsNull( i ) ? DB::Value{} : DB::Value{ ToString(i) }; }
		α ToString( uint i )Ι->string override{ const char* p=base::_pBuffer.get(); return base::IsNull( i ) ? string{} : string{ p+base::Size()*i, p+base::Size()*i+base::Output(i) }; }
		α MoveString( uint i )ι->string{ auto result = ToString( i ); base::_pBuffer.reset(); return result; }

		α BufferLength()Ι->SQLLEN override{ return base::Size(); }
	};

	struct BindingBits final: TBindings<bool,SQL_BIT, SQL_C_BIT>
	{
		BindingBits( uint rowCount ):TBindings<bool,SQL_BIT, SQL_C_BIT>{ rowCount }{}//-7
		α Object( uint i )Ι->Value override{ return Value{ Bit(i) }; }
		α Bit( uint i )Ι->bool override{ return _pBuffer[i]!=0; }
		α Int( uint i )Ι->int64_t override{ return static_cast<int64_t>(Bit(i)); }
		α ToString( uint i )Ι->string override{ return Bit( i ) ? "true" : "false"; }
	};

	struct BindingInt32s final: TBindings<int,SQL_INTEGER, SQL_C_SLONG>
	{
		BindingInt32s( uint rowCount ): TBindings<int,SQL_INTEGER, SQL_C_SLONG>{ rowCount }{}
		α Object( uint i )Ι->Value override{ return Value{Int32(i)}; }
		α Int32( uint i )Ι->int32_t override{ return _pBuffer[i]; }
		α Int( uint i )Ι->int64_t override{ return Int32(i); }
		α UInt( uint i )Ι->uint override{ return static_cast<uint>(Int32(i)); }
		α IntOpt( uint i )Ι->optional<_int>{ optional<_int> value; if( !IsNull(i) )value=Int( i ); return value; }
		α UIntOpt( uint i )Ι->optional<uint> override{ optional<uint> optional; if( !IsNull(i) ) optional = UInt( i ); return optional; };
	};
	
	struct BindingInts final: TBindings<_int,SQL_BIGINT, SQL_C_SBIGINT>
	{
		BindingInts( uint rowCount ): TBindings<_int,SQL_BIGINT, SQL_C_SBIGINT>{ rowCount }{}
		α Object( uint i )Ι->Value override{ return Value{Int(i)}; };
		α Int( uint i )Ι->int64_t override{ return _pBuffer[i]; }
		α UInt( uint i )Ι->uint override{ return static_cast<uint>( Int(i) ); }
		α UIntOpt( uint i )Ι->optional<uint>{ optional<uint> value; if(!IsNull(i))value=UInt( i ); return value; };
		α DateTime( uint i )Ι->DBTimePoint{ return Clock::from_time_t(Int(i)); }
	};
#pragma warning(push)
#pragma warning(disable:4005)
#define base TBindings<SQL_TIMESTAMP_STRUCT, TSql, SQL_C_TYPE_TIMESTAMP>
	template<SQLSMALLINT TSql>
	struct BindingTimes : base
	{
		BindingTimes( uint rowCount ):base{ rowCount }{}
		
		α Object( uint i )Ι->Value override{ return base::IsNull( i ) ? Value{ nullptr } : Value{ DateTime(i) }; }
		α BufferLength()Ι->SQLLEN override{ return sizeof(SQL_TIMESTAMP_STRUCT); }
		α Size()Ι->SQLULEN override{ return 27; }//https://wezfurlong.org/blog/2005/Nov/calling-sqlbindparameter-to-bind-sql-timestamp-struct-as-sql-c-type-timestamp-avoiding-a-datetime-overflow/
		α DecimalDigits()Ι->SQLSMALLINT override{ return 7; }
		α DateTime( uint i )Ι->DBTimePoint
		{
			if( base::IsNull(i) ) return DBTimePoint{};
			SQL_TIMESTAMP_STRUCT data = base::_pBuffer[i];
			return Chrono::ToTimePoint( data.year, (uint8)data.month, (uint8)data.day, (uint8)data.hour, (uint8)data.minute, (uint8)data.second, Duration(data.fraction) );
		}
		α DateTimeOpt( uint i )Ι->optional<DBTimePoint> override{ return base::IsNull(i) ? nullopt : std::make_optional(DateTime(i)); }
	};
	////////////////////////////////////////////////////////////////////////////
	#define base TBindings<double, TSql, SQL_C_DOUBLE>
	template<SQLSMALLINT TSql>
	struct BindingDoubles : base
	{
		BindingDoubles( uint rowCount )ι:base{ rowCount }{}

		α Object( uint i )Ι->Value override{ return base::IsNull(i) ? Value{} : Value{ Double(i) }; }
		α Double( uint i )Ι->double override{ return base::_pBuffer[i]; }
		α DoubleOpt( uint i )Ι->optional<double>{ optional<double> value; if( !base::IsNull(i) ) value = Double(i); return value; }
	};
	////////////////////////////////////////////////////////////////////////////
	#define base TBindings<SQL_NUMERIC_STRUCT,SQL_NUMERIC,SQL_C_NUMERIC>
	#define var const auto
	struct BindingNumerics : public base{
		BindingNumerics( uint rowCount )ι:base{ rowCount }{}
		α Object( uint i )Ι->Value override{ return Value{ Double(i) }; }
		α Double( uint index )Ι->double override{//https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/retrieve-numeric-data-sql-numeric-struct-kb222831?view=sql-server-ver15
			var data = _pBuffer[index];
			uint divisor = (uint)std::pow( 1, data.scale );
			_int value = 0, last=1;
			for( uint i=0; i<SQL_MAX_NUMERIC_LEN; ++i ){
				const int current = data.val[i];
				const int a = current % 16;
				const int b = current / 16;
				value += last * a;   
				last *= 16;
	         value += last * b;
				last *= 16;
			}
			return (data.sign ? 1 : -1)*(double)value/divisor; 
		}
		α DoubleOpt( uint i )Ι->optional<double> override{ optional<double> value; if( !IsNull(i) ) value = Double(i); return value; }
		α Int( uint i )Ι->_int override{ return (_int)Double( i ); }
	};

	////////////////////////////////////////////////////////////////////////////
	#define base TBindings<float, SQL_FLOAT, SQL_C_FLOAT>
	struct BindingFloats : base
	{
		BindingFloats( uint rowCount ): base{ rowCount }{}

		α Object( uint i )Ι->Value override{ return Value{ Double(i) }; }
		α Double( uint i )Ι->double override{ return _pBuffer[i]; }
		α DoubleOpt( uint i )Ι->optional<double>{ return IsNull(i) ? nullopt : optional<double>{ Double(i) }; }
	};

	////////////////////////////////////////////////////////////////////////////
	#define base TBindings<float, SQL_SMALLINT, SQL_C_SSHORT>
	struct BindingInt16s : base
	{
		BindingInt16s( uint rowCount ): base{ rowCount }{}

		α Object( uint i )Ι->Value override{ return IsNull( i ) ? Value{ nullptr } : Value{ Int(i) }; }
		α UInt( uint i )Ι->uint override{ return static_cast<uint>( Int(i) ); }
		α Int( uint i )Ι->_int override{ return static_cast<_int>( _pBuffer[i] ); }
		α IntOpt( uint i )Ι->optional<_int> override{ return IsNull(i) ? nullopt : optional<_int>( Int(i) ); }
		α Double( uint i )Ι->double override{ return (double)Int(i); }
		α DoubleOpt( uint i )Ι->optional<double>{ return IsNull(i) ? nullopt : optional<double>( Double(i) ); }
	};
////////////////////////////////////////////////////////////////////////////
	#define base TBindings<uint8_t, SQL_TINYINT, SQL_C_TINYINT>
	struct BindingInt8s : base
	{
		BindingInt8s( uint rowCount ): base{ rowCount }{}

		α Object( uint i )Ι->Value override{ return Value{ UInt(i) }; }

		α UInt( uint i )Ι->uint override{ return static_cast<uint>( _pBuffer[i] ); }
		α UIntOpt( uint i )Ι->optional<uint> override{ optional<uint> value; if( !IsNull(i) ) value = UInt( i ); return value; }
		α IntOpt( uint i )Ι->optional<_int> override{ optional<_int> value; if( !IsNull(i) ) value = Int( i ); return value; }
		α Int( uint i )Ι->int64_t override{ return static_cast<int64_t>( UInt(i) ); }
		α Int32( uint i )Ι->int32_t{ return (int32_t)Int(i); }
		α Double( uint i )Ι->double override{ return (double)UInt( i ); }
		α DoubleOpt( uint i )Ι->optional<double> override{ optional<double> value; if( !IsNull(i) ) value = Double( i ); return value; }
	};

	Ξ IBindings::Create( SQLSMALLINT type, uint rowCount )ε->up<IBindings>
	{
		up<IBindings> pBinding;
		if( type==BindingBits::SqlType() )
			pBinding = mu<BindingBits>( rowCount );
		else if( type==BindingInt8s::SqlType() )
			pBinding = mu<BindingInt8s>( rowCount );
		else if( type==BindingInt32s::SqlType() )
			pBinding = mu<BindingInt32s>( rowCount );
		//else if( type==SQL_DECIMAL )
		//	pBinding = mu<BindingDouble>( rowCount );
		else if( type==BindingInt16s::SqlType() )
			pBinding = mu<BindingInt16s>( rowCount );
		else if( type==BindingFloats::SqlType() )
			pBinding = mu<BindingFloats>( rowCount );
		else if( type==SQL_REAL )
			pBinding = mu<BindingDoubles<SQL_REAL>>( rowCount );
		else if( type==SQL_DOUBLE )
			pBinding = mu<BindingDoubles<SQL_DOUBLE>>( rowCount );
		else if( type==SQL_DATETIME )
			pBinding = mu<BindingTimes<SQL_DATETIME>>( rowCount );
		else if( type==BindingInts::SqlType() )
			pBinding = mu<BindingInts>( rowCount );
		else if( type==SQL_TYPE_TIMESTAMP )
			pBinding = mu<BindingTimes<SQL_TYPE_TIMESTAMP>>( rowCount );
		else if( type==BindingNumerics::SqlType() )
			pBinding = mu<BindingNumerics>( rowCount );
		else
			THROW( "Binding type '{}' is not implemented.", type );
		return pBinding;
	}
	Ξ IBindings::Create( SQLSMALLINT type, uint rowCount, uint size )ε->up<IBindings>
	{
		up<IBindings> p;
		if( type==SQL_CHAR )
			p = mu<BindingStrings<SQL_CHAR>>( rowCount, size );
		else if( type==SQL_VARCHAR )
			p = mu<BindingStrings<SQL_VARCHAR>>( rowCount, size );
		else if( type==SQL_LONGVARCHAR )
			p = mu<BindingStrings<SQL_LONGVARCHAR>>( rowCount, size );
		else if( type==-9 )//varchar(max)
			p = mu<BindingStrings<SQL_LONGVARCHAR>>( rowCount, size );
		else
			THROW( "Binding type '{}' is not implemented.", type );
		return p;	
	}
#undef base
#undef var
#pragma warning(pop)
}