#pragma once
#include <jde/db/meta/Column.h>

namespace Jde::DB{
	struct ColumnDdl final: Column{
		ColumnDdl( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isSequence, optional<uint8> skIndex, optional<uint> numericPrecision, optional<uint> numericScale )ι;

		α CreateStatement( const Syntax& syntax )Ε->string;
		α DataTypeString( const Syntax& syntax )Ι->string;
	};
}