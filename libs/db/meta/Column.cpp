#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/Schema.h>

#define let const auto

namespace Jde::DB{
	α GetDefault( const jobject& j, bool isNullable, EType type, str name )ε->optional<Value>{
		optional<Value> obj;
		if( j.contains("default") )
			obj = Value{ type, j.at("default") };
		else if( isNullable )
			obj = Value{};
		return obj;
	}

	Column::Column( sv name )ι:
		Name{ name }
	{}
	using namespace Json;
	Column::Column( sv name, const jobject& j )ε:
		Name{ name },
		Criteria{ FindDefaultSV(j, "criteria") },
		Insertable{ FindBool(j, "insertable").value_or(true) },
		IsSequence{ FindDefaultBool(j, "sequence") },
		SKIndex{ j.contains("type") ? FindNumber<uint8>(j, "type") : IsSequence ? optional<uint8>{1} : optional<uint8>{} },
		IsNullable{ FindDefaultBool(j, "nullable") },
		MaxLength{ FindNumber<uint16>(j, "length") },
		PKTable{ FindSV(j, "pkTable") ? ms<DB::Table>(j.at("pkTable").as_string()) : sp<DB::Table>{} },
		QLAppend{ FindDefaultSV(j, "qlAppend") },
		Type{ j.contains("type") ? ToType(AsString(j.at("type")))  : EType::VarChar },
		Updateable{ FindBool(j, "updateable").value_or(true) },
		Default{ GetDefault(j, IsNullable, Type, Name) }
	{}

	α Column::Initialize( sp<DB::Table> table )ι->void{
		Table=table;
		if( PKTable ){
			PKTable = table->Schema->GetTablePtr( PKTable->Name );
		}
	}
	α Column::FQName()Ι->string{
		return Ƒ( "{}.{}", Table->DBName, Name );
	}

	α Column::IsEnum()Ι->bool{
		return PKTable && PKTable->IsEnum();
	}

	α Column::IsFlags()Ι->bool{
		return PKTable && PKTable->IsFlags;
	}
	α Column::IsPK()Ι->bool{
		return Table->SurrogateKeys.size()==1 && SKIndex.has_value();
	}

	α Column::View()Ι->sp<DB::View>{
		return dynamic_pointer_cast<DB::View>(Table);
	}
}