#include <jde/db/metadata/Column.h>

namespace Jde::DB{
	Column::Column( sv name, uint ordinalPosition, sv dflt, bool isNullable, EType type, optional<uint> maxLength, bool isIdentity, bool isId, optional<uint> numericPrecision, optional<uint> numericScale )ι:
		Name{name},
		OrdinalPosition{ordinalPosition},
		Default{dflt},
		IsNullable{isNullable},
		Type{type},
		MaxLength{maxLength},
		IsIdentity{isIdentity},
		IsId{isId},
		NumericPrecision{numericPrecision},
		NumericScale{numericScale}
	{}

	Column::Column( sv name )ι:
		Name{ name }
	{}

	Column::Column( sv name, const nlohmann::json& j, const flat_map<string,Column>& commonColumns, const flat_map<string,Table>& parents, const nlohmann::ordered_json& schema )ε:
		Name{ name }{
		auto getType = [this,&commonColumns, &schema, &parents]( sv typeName ){
			IsNullable = typeName.ends_with( "?" );
			if( IsNullable )
				typeName = typeName.substr( 0, typeName.size()-1 );
			if( var p = commonColumns.find(string{typeName}); p!=commonColumns.end() ){
				var name = Name; var isNullable = IsNullable;
				*this = p->second;
				Name = name;
				IsNullable = isNullable;
			}
			else{
				Type = ToType( ToIV(typeName) );
				if( Type==EType::None ){
					if( var pPKTable = schema.find(string{typeName}); pPKTable!=schema.end() ){
						Table table{ typeName, *pPKTable, parents, commonColumns, schema };
						if( var pColumn = table.SurrogateKeys.size()==1 ? &table.SurrogateKey() : nullptr; pColumn )
							Type = pColumn->Type;
						IsEnum = table.Data.size();
					}
					if( Type==EType::None )
						Type = EType::UInt;
					PKTable = Schema::FromJson( typeName );
				}
			}
		};
		if( j.is_object() ){
			if( j.contains("sequence") ){
				IsIdentity = j.find( "sequence" )->get<bool>();
				if( IsIdentity )
					IsNullable = Insertable = Updateable = false;
			}
			else{
				if( j.contains("type") )
					getType( j.find("type")->get<string>() );
				if( j.contains("length") ){
					MaxLength = j.find( "length" )->get<uint>();
					if( Type!=EType::Char )
						Type = EType::VarTChar;
				}
				if( j.contains("default") )
					Default = j.find("default")->get<string>();
				if( j.contains("insertable") )
					Insertable = j.find("insertable")->get<bool>();
				if( j.contains("updateable") )
					Updateable = j.find("updateable")->get<bool>();
				if( j.contains("qlAppend") )
					QLAppend = j.find("qlAppend")->get<string>();
				if( j.contains("criteria") )
					Criteria = j.find("criteria")->get<string>();
			}
		}
		else if( j.is_string() )
			getType( j.get<string>() );
	}
	α Column::Initiaize( sp<Table> table )ι->void{ Table=table; }


	α Column::DataTypeString( const Syntax& syntax )Ι->string{
		return MaxLength ? Jde::format("{}({})", ToStr(ToString(Type, syntax)), *MaxLength) : ToStr( ToString(Type, syntax) );
	}
	α Column::DefaultObject()Ι->DB::object{
		object obj;
		if( Default.size() ){
			if( Type==EType::Bit )
				obj = Default=="1" || ToIV(Default)=="true" ? 1 : 0;
			else
				obj = Default;
		}
		return obj;
	}

	α Column::Create( const Syntax& syntax )Ι->string{
		var null = IsNullable ? "null"sv : "not null"sv;
		const string sequence = IsIdentity ?  " "+string{syntax.IdentityColumnSyntax()} : string{};
		string dflt;
		if( Default.size() ){
			if( Type==EType::Bit )
				dflt = Jde::format( " default {}", Default=="1" || ToIV(Default)=="true" ? 1 : 0 );
			else
				dflt = Jde::format( " default {}", Default=="$now" ? ToStr(syntax.NowDefault()) : Jde::format("'{}'", Default) );
		}
		return Jde::format( "{} {} {}{}{}", Name, DataTypeString(syntax), null, sequence, dflt );
	}
}