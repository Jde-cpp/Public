#pragma once
#include <jde/db/meta/Column.h>

namespace Jde::DB{
	struct ΓDB ColumnDdl final: Column{
		ColumnDdl( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isSequence, optional<uint8> skIndex, optional<uint> numericPrecision, optional<uint> numericScale )ι;
		//#39: a bool in skIndex's place converts to an *engaged* optional{0} - "column 0 of the primary key", not "no
		//primary key" - and it does so silently, right after a bool parameter that is spelled the same way at the call
		//site.  Deleting the all-bool spelling makes that a compile error instead of a wrong answer.
		ColumnDdl( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isSequence, bool skIndex, optional<uint> numericPrecision, optional<uint> numericScale )ι = delete;

		Ω CreateStatement( const Column& config )ε->string;
		Ω DataTypeString( const Column& config )ι->string;
	};
}