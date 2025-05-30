#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/framework/str.h>

#define let const auto

namespace Jde::DB{
	constexpr array<sv,3> CardinalityStrings = { "0", "1", "N" };

	α Column::Count()ι->sp<Column>{ return ms<Column>( "count(*)" ); }

	α getDefault( const jobject& j, bool isNullable, EType type/*, sv name*/ )ε->optional<Value>{
		optional<Value> obj;
		if( j.contains("default") )
			obj = Value{ type, j.at("default") };
		else if( isNullable )
			obj = type == EType::VarBinary ? Value{vector<byte>()} : Value{};  //odbc handles varbinary nulls differently.
		return obj;
	}

	α getCriteria( const jobject& config )ε->optional<DB::Criteria> {
		optional<DB::Criteria> criteria;
		if( auto j = Json::FindObject( config, "criteria" ); j )
			criteria = { ms<DB::Column>( Json::AsSV(*j, "columnName") ),  Value{serialize(Json::AsValue(*j, "value"))} };
		return criteria;
	}


	α getPK( const jobject& j )ε->sp<Table>{
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
		Criteria{ getCriteria(j) },
		IsNullable{ FindDefaultBool(j, "nullable") },
		MaxLength{ FindNumber<uint16>(j, "length") },
		PKTable{ getPK(j) },
		IsSequence{ PKTable ? false : FindDefaultBool(j, "sequence") },
		Insertable{ FindBool(j, "insertable").value_or(!IsSequence) },
		SKIndex{ IsSequence ? optional<uint8>{0} : FindNumber<uint8>(j, "sk") },
		QLAppend{ FindDefaultSV(j, "qlAppend") },
		Type{ ToType(FindSV(j, "type").value_or("VarChar")) },
		Updateable{ FindBool(j, "updateable").value_or(true) },
		Default{ getDefault(j, IsNullable, Type) }
	{}

	α Column::Initialize( sp<DB::View> view )ε->void{
		Table = view;
		if( PKTable ){
			PKTable = view->Schema->GetTablePtr( PKTable->Name );
		}
		if( let table = Criteria ? dynamic_pointer_cast<DB::Table>(view) : sp<DB::Table>{}; table && table->Extends ){
			Criteria->Column = table->Extends->GetColumnPtr( Criteria->Column->Name );
			if( Criteria->Value.Type()==EValue::String )//physical table will have config value, not json from constructor.
				Criteria->Value = { Criteria->Column->Type, Json::ParseValue(move(Criteria->Value.move_string())) };
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
}