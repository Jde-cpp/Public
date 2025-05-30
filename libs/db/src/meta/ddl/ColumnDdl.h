#pragma once
#include <jde/db/meta/Column.h>

namespace Jde::DB{
	struct ColumnDdl final: Column{
		ColumnDdl( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isSequence, optional<uint8> skIndex, optional<uint> numericPrecision, optional<uint> numericScale )ι;

		Ω CreateStatement( const Column& config )ε->string;
		Ω DataTypeString( const Column& config )ι->string;
	};
}