#pragma once
#include "../exports.h"
#include "../Value.h"

namespace Jde::DB{
	struct Table; struct View;

	struct ΓDB Column{
		Column()=default;
		Column( sv name )ι;  //placeholder
		Column( sv name, const jobject& j )ε;
		virtual ~Column()=default;

		α FQName()Ι->string;
		α IsEnum()Ι->bool; //cache=true
		α IsFlags()Ι->bool;
		α Initialize( sp<DB::Table> table )ι->void;
		α IsPK()Ι->bool;  //surrogate keys==1 && SKIndex==0
		α View()Ι->sp<DB::View>;

		string Name;

		string Criteria;//unUsers=not is_group
		bool Insertable;
		bool IsSequence; //uses db sequence.  TODO look to move to ddl.
		optional<uint8> SKIndex; //Is part of the surrogate key for the table.
		bool IsNullable;
		optional<uint> MaxLength;
		optional<uint> NumericPrecision; //currently for db schema columns
		optional<uint> NumericScale; //currently for db schema columns
		sp<DB::Table> PKTable; //pk table if any.
		string QLAppend; //also select this column in ql query TODO example
		sp<DB::Table> Table;
		EType Type;
		bool Updateable;
		optional<Value> Default;
	};
}