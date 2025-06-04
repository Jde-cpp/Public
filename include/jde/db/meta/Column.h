#pragma once
#include "../exports.h"
#include "../Value.h"

namespace Jde::DB{
	struct Column; struct Table; struct View;
	enum class ECardinality : uint8{ Zero=0, One=1, Many=2 };
	struct Criteria {
		sp<DB::Column> Column;
		DB::Value Value;
	};

	struct ΓDB Column{
		Column()=default;
		Column( sv name )ι;  //placeholder
		Column( sv name, const jobject& j )ε;
		virtual ~Column()=default;

		Ω Count()ι->sp<Column>;
		α FQName()Ι->string;
		α IsEnum()Ι->bool; //cache=true
		α IsFlags()Ι->bool;
		α Initialize( sp<DB::View> view )ε->void;
		α IsPK()Ι->bool;  //surrogate keys==1 && SKIndex==0

		string Name;

		optional<DB::Criteria> Criteria;//unUsers=not is_group
		bool IsNullable;
		optional<uint> MaxLength;
		optional<uint> NumericPrecision; //currently for db schema columns
		optional<uint> NumericScale; //currently for db schema columns
		sp<DB::View> PKTable; //pk table if any.
		//tuple<ECardinality,ECardinality> PKCardinality;

		bool IsSequence; //uses db sequence.  TODO look to move to ddl.
		bool Insertable;
		optional<uint8> SKIndex; //Is part of the surrogate key for the table.
		string QLAppend; //also select this column in ql query TODO example
		sp<DB::View> Table;
		EType Type;
		bool Updateable;
		optional<Value> Default; //nullable=Value{}
	};
}