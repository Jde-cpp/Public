#include <jde/db/Row.h>

namespace Jde::DB{
	α Row::GetBit( uint i )Ι->bool{ return _values[i].Type()==EValue::Bool ? _values[i].get_bool() : _values[i].get_number<uint8>(); }
	α Row::GetBitOpt( uint i )Ι->optional<bool>{ return IsNull(i) ? optional<bool>{} : _values[i].Type()==EValue::Bool ? _values[i].get_bool() : _values[i].get_number<uint8>(); }
	α Row::IsNull( uint i )Ι->bool{ return _values[i].Type() == EValue::Null; }
}