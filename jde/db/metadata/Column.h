#pragma once
#include "../exports.h"
#include "../DataType.h"

namespace Jde::DB{
	struct Table;

	struct ΓDB Column final{
		Column()=default;
		Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )ι;
		Column( sv name )ι;
		Column( const jobject& j, bool isSurrogateKey )ε;
		α Initiaize( sp<Table> table )ι->void;
		α Create( const Syntax& syntax )Ι->string;
		α DataTypeString( const Syntax& syntax )Ι->string;
		α DefaultObject()Ι->DB::object;

		string Name;
		uint OrdinalPosition;
		string Default;
		bool IsNullable{};
		mutable bool IsFlags{};
		mutable bool IsEnum{};
		sp<DB::Table> Table;
		EType Type{ EType::UInt };
		optional<uint> MaxLength;
		bool IsIdentity{};
		bool IsId{};
		optional<uint> NumericPrecision;
		optional<uint> NumericScale;
		bool Insertable{ true };
		bool Updateable{ true };
		string PKTable;
		string QLAppend;//also select this column in ql query
		string Criteria;//unUsers=not is_group
	};
}