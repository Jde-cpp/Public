#include <jde/ql/types/Introspection.h>
#include <jde/framework/io/json.h>
#include <jde/ql/types/TableQL.h>
#include <jde/db/names.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::QL{
	using namespace Json;
	α GetTable( str tableName )ε->sp<DB::View>;
	Introspection _introspection;
	α SetIntrospection( Introspection&& x )ι->void{ _introspection = move(x); }

	constexpr array<sv,8> FieldKindStrings = { "SCALAR", "OBJECT", "INTERFACE", "UNION", "ENUM", "INPUT_OBJECT", "LIST", "NON_NULL" };
	α ToFieldKind( sv x ){ return ToEnum<EFieldKind>( FieldKindStrings, x ); }
	α ToString( EFieldKind x ){ return FromEnum<EFieldKind>( FieldKindStrings, x ); }

	Type::Type( const jobject& j )ε:
		Name{ FindDefaultSV(j, "name") },
		Kind{ FindEnum<EFieldKind>( j,"kind", ToFieldKind ).value_or( EFieldKind::Scalar ) },
		IsNullable{ Kind!=EFieldKind::NonNull }{
		if( !IsNullable ){
			const jobject& type = AsObject( j, "ofType" );
			Name = AsString( type, "name" );
			Kind = FindEnum<EFieldKind>( type,"kind", ToFieldKind ).value_or( EFieldKind::Scalar );
			if( type.contains("ofType") )
				OfTypePtr = mu<Type>( AsObject(type, "ofType") );
		}
		else if( j.contains("ofType") )
			OfTypePtr = mu<Type>( AsObject(j, "ofType") );
	}
	α Type::ToJson( const TableQL& query, bool ignoreNull )Ε->jobject{
		jobject jType;
		bool isNullType = ignoreNull || IsNullable;
		if( auto pColumn = query.FindColumn("name"); pColumn ){
			if( isNullType && Name.size() )
				jType["name"] = Name;
			else
				jType["name"] = nullptr;
		}
		if( auto pColumn = query.FindColumn("kind"); pColumn )
			jType["kind"] = ToString( isNullType ? Kind : EFieldKind::NonNull );
		auto pOfType = !IsNullable && !ignoreNull ? this : OfTypePtr.get();
		if( auto pTable = pOfType ? query.FindTable("ofType") : nullptr; pTable )
			jType["ofType"] = pOfType->ToJson( *pTable, pOfType==this );
		return jType;
	}

	Field::Field( const jobject& j )ε:
		Name{ AsSV(j, "name") },
		FieldType{ AsObject(j, "type") }
	{}

	α Field::ToJson( const TableQL& query )Ε->jobject{
		jobject jField;
		if( auto pColumn = query.FindColumn("name"); pColumn )
			jField["name"] = Name;
		if( auto pTable = query.FindTable("type"); pTable )
			jField["type"] = FieldType.ToJson( *pTable, false );
		return jField;
	}

	Object::Object( sv name, const jobject& j )ε:
		Name{ name }{
		for( let field : AsArray(j, "fields") ){
			Fields.emplace_back( AsObject(field) );
		}
	}

	α Object::ToJson( const TableQL& query )Ε->jobject{
		ASSERT( query.JsonName=="fields" );
		jobject jTable;
		jTable["name"] = Name;
		jarray fields;
		for( let& field : Fields )
			fields.push_back( field.ToJson(query) );
		jTable["fields"] = fields;
		return jTable;
	}

	Introspection::Introspection( const jobject&& j )ε{
		for( let& [name, value]  : j )
			Objects.emplace_back( name, AsObject(value) );
	}

	α Introspection::Find( sv name )Ι->const Object*{
		auto y = find_if( Objects, [name](let& Value){ return Value.Name==name; } );
		return y==Objects.end() ? nullptr : &*y;
	}

	using namespace DB::Names;
	α IntrospectFields( sv /*typeName*/, const DB::Table& mainTable, const TableQL& fieldTable, jobject& jData )ε->void{
		jarray fields;
		let pTypeTable = fieldTable.FindTable( "type" );
		let haveName = fieldTable.FindColumn( "name" )!=nullptr;
		let pOfTypeTable = pTypeTable->FindTable( "ofType" );
		jobject jTable;
		jTable["name"] = mainTable.JsonTypeName();

		auto addField = [&]( sv name, sv typeName, EFieldKind typeKind, sv ofTypeName, optional<EFieldKind> ofTypeKind ){
			jobject field;
			if( haveName )
				field["name"] = name;
			if( pTypeTable ){
				jobject type;
				auto setField = []( const TableQL& t, jobject& j, str key, sv x ){ if( t.FindColumn(key) ){ if(x.size()) j[key]=x; else j[key]=nullptr; } };
				auto setKind = []( const TableQL& t, jobject& j, optional<EFieldKind> pKind ){
					if( t.FindColumn("kind") ){
						if( pKind )
							j["kind"] = ToString( *pKind );
						else
							j["kind"] = nullptr;
					}
				};
				setField( *pTypeTable, type, "name", typeName );
				setKind( *pTypeTable, type, typeKind );
				if( pOfTypeTable && (ofTypeName.size() || ofTypeKind) ){
					jobject ofType;
					setField( *pOfTypeTable, ofType, "name", ofTypeName );
					setKind( *pOfTypeTable, ofType, ofTypeKind );
					type["ofType"] = ofType;
				}
				field["type"] = type;
			}
			fields.push_back( field );
		};
		function<void(const DB::View&, bool, string)> addColumns = [&addColumns,&addField,&mainTable]( const DB::View& dbTable, bool isMap, string prefix={} ){
			for( let& c : dbTable.Columns ){
				let& column = *c;
//				BREAK_IF( column.Name=="provider_id" );
				string fieldName;
				string qlTypeName;
				auto rootType{ EFieldKind::Scalar };
				if( !column.PKTable ){
					if( column.Type==DB::EType::VarBinary || (prefix.size() && column.SKIndex) )//use NID see RolePermission's permissions, varbinary use case is passwords
						continue;
					fieldName = column.IsPK() ? "id" : ToJson( column.Name );
					qlTypeName = ColumnQL::QLType( column );//column.PKTable.empty() ? ColumnQL::QLType( column ) : dbTable.JsonTypeName();
				}
				else if( column.PKTable ){
					auto pChildColumn = dbTable.Map ? dbTable.Map->Child : nullptr;
					if( !isMap || column.PKTable->IsFlags || (pChildColumn && pChildColumn->Name==column.Name)  ){ //!RolePermission || right_id || um_groups.member_id
						if( find_if(dbTable.Columns, [&column](let& c){return c->QLAppend==column.Name;})!=dbTable.Columns.end() )
							continue;
						if( mainTable.Extends && mainTable.Extends->GetPK()->Name==column.Name ){//extension table
							addColumns( *column.PKTable, false, prefix );
							continue;
						}
						qlTypeName = column.PKTable->JsonTypeName();
						if( column.PKTable->IsFlags ){
							fieldName = ToPlural<sv>( fieldName );
							rootType = EFieldKind::List;
						}
						else if( pChildColumn ){
							fieldName = ToPlural<sv>( ToJson(Str::Replace(column.Name, "_id", "")) );
							rootType = EFieldKind::List;
						}
						else{
							fieldName = ToJson<sv>( qlTypeName );
							rootType = column.PKTable->IsEnum() ? EFieldKind::Enum : EFieldKind::Object;
						}
					}
					else{ //isMap
						//if( !typeName.starts_with(pPKTable->JsonTypeName()) )//typeName==RolePermission, don't want role columns, just permissions.
						addColumns( *column.PKTable, false, column.PKTable->JsonTypeName() );
						continue;
					}
				}
				else
					THROW( "[{}]Could not find table.", column.PKTable->Name );

				auto pChildColumn = dbTable.Map ? dbTable.Map->Child : nullptr;
				let isNullable = pChildColumn || column.IsNullable;
				let typeName2 = isNullable ? qlTypeName : "";
				let typeKind = isNullable ? rootType : EFieldKind::NonNull;
				let ofTypeName = isNullable ? "" : qlTypeName;
				let ofTypeKind = isNullable ? optional<EFieldKind>{} : rootType;

				addField( fieldName, typeName2, typeKind, ofTypeName, ofTypeKind );
			}
		};
		addColumns( mainTable, mainTable.Map.has_value(), {} );
		for( let& [name,pTable] : mainTable.Schema->Tables ){
			auto fnctn = [addField,pTable,&mainTable]( let& c1Name, let& c2Name ){
				if( let pColumn1=pTable->FindColumn(c1Name), pColumn2=pTable->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ ){
					if( pColumn1->PKTable->Name==mainTable.Name ){
						let pTable2 = pColumn2->PKTable;
						let jsonType = pTable->Columns.size()==2 ? pTable2->JsonTypeName() : pTable->JsonTypeName();
						addField( ToPlural<string>(ToJson<sv>(jsonType)), {}, EFieldKind::List, jsonType, EFieldKind::Object );
					}
				}
			};
			let child = pTable->Map ? pTable->Map->Child : nullptr;
			let parent = pTable->Map ? pTable->Map->Parent : nullptr;
			if( child && parent ){
				fnctn( child->Name, parent->Name );
				fnctn( parent->Name, child->Name );
			}
		}
		jTable["fields"] = fields;
		jData["__type"] = jTable;
	}

α IntrospectEnum( const sp<DB::Table> baseTable, const TableQL& fieldTable, jobject& jData )ε->jobject{
		auto dbTable = baseTable->QLView ? baseTable->QLView : AsView(baseTable);
		DB::SelectClause select;
		for_each( fieldTable.Columns, [&select, &dbTable](let& x){
			if( let c = x.JsonName=="id" ? dbTable->GetPK() : dbTable->FindColumn( x.JsonName ); c )
				select.TryAdd( c );
		});//sb only id/name.
		const string sql{ Ƒ("{} from {} order by {}", select.ToString(), dbTable->Name, dbTable->GetPK()->Name) };
		jarray fields;
		dbTable->Schema->DS()->Select( sql, [&]( DB::IRow& row ){
			jobject j;
			for( uint i=0; i<select.Columns.size(); ++i ){
				auto& c = *select.Columns[i];
				if( c.IsPK() )
					j["id"] = row.GetUInt( i );
				else
					j[c.Name] = row.MoveString( i );
			}
			fields.push_back( j );
		} );
		jobject jTable;
		jTable["enumValues"] = fields;
		return jTable;
	}

	α QueryType( const TableQL& typeTable, jobject& jData )ε->void{
		let typeName = Json::AsString( typeTable.Args, "name" );
		auto dbTable = DB::AsTable( GetTable(ToPlural(FromJson(typeName))) );
		for( let& qlTable : typeTable.Tables ){
			if( qlTable.JsonName=="fields" ){
				if( let pObject = _introspection.Find(typeName); pObject )
					jData["__type"] = pObject->ToJson( qlTable );
				else
					IntrospectFields( typeName, *dbTable, qlTable, jData );
			}
			else if( qlTable.JsonName=="enumValues" )
				jData["__type"] = IntrospectEnum( dbTable, qlTable, jData );
			else
				THROW( "__type data for '{}' not supported", qlTable.JsonName );
		}
	}
}
