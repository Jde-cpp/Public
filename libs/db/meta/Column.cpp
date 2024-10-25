#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/AppSchema.h>

#define let const auto

namespace Jde::DB{
	α Column::Count()ι->sp<Column>{ return ms<Column>( "count(*)" ); }
	α GetDefault( const jobject& j, bool isNullable, EType type, str name )ε->optional<Value>{
		optional<Value> obj;
		if( j.contains("default") )
			obj = Value{ type, j.at("default") };
		else if( isNullable )
			obj = Value{};
		return obj;
	}

	α GetPK( const jobject& j )ε->sp<Table>{
		sp<Table> table;
		if( auto name = Json::FindSV(j, "pkTable"); name )
			table = ms<Table>( *name );
		else if( let pk = Json::FindObject(j, "pkTable"); pk )
			table = ms<Table>( Json::AsSV(*pk,"name") );
		return table;
	}

	Column::Column( sv name )ι:
		Name{ name }
	{}
	using namespace Json;
	Column::Column( sv name, const jobject& j )ε:
		Name{ name },
		Criteria{ FindDefaultSV(j, "criteria") },
		IsNullable{ FindDefaultBool(j, "nullable") },
		MaxLength{ FindNumber<uint16>(j, "length") },
		PKTable{ GetPK(j) },
		IsSequence{ PKTable ? false : FindDefaultBool(j, "sequence") },
		Insertable{ FindBool(j, "insertable").value_or(!IsSequence) },
		SKIndex{ IsSequence ? optional<uint8>{0} : FindNumber<uint8>(j, "sk") },
		QLAppend{ FindDefaultSV(j, "qlAppend") },
		Type{ ToType(FindSV(j, "type").value_or("VarChar")) },
		Updateable{ FindBool(j, "updateable").value_or(true) },
		Default{ GetDefault(j, IsNullable, Type, Name) }
	{}

	α Column::Initialize( sp<DB::View> table )ε->void{
		Table=table;
		if( PKTable )
			PKTable = table->Schema->GetTablePtr( PKTable->Name );
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

	//α Column::View()Ι->sp<DB::View>{
	//	return dynamic_pointer_cast<DB::View>(Table);
	//}
}