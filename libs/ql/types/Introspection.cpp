#include <jde/ql/types/Introspection.h>
#include <jde/framework/io/json.h>
#include <jde/ql/types/TableQL.h>
#include <jde/db/names.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::QL{
	using namespace Json;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	Introspection _introspection;
	α AddIntrospection( Introspection&& x )ι->void{ _introspection += move(x); }

	α Introspection::operator+=( Introspection&& x )ι->void{
		for( auto&& o : x.Objects )
			Objects.emplace_back( move(o) );
	}

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
		for( let& field : AsArray(j, "fields") ){
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
	α IntrospectFields( sv /*typeName*/, const DB::Table& mainTable, const TableQL& fieldTable )ε->jobject{
		jarray fields;
		let pTypeTable = fieldTable.FindTable( "type" );
		let haveName = fieldTable.FindColumn( "name" )!=nullptr;
		let pOfTypeTable = pTypeTable->FindTable( "ofType" );
		jobject jTable;
		jTable["name"] = mainTable.JsonName();

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
				string fieldName;
				string qlTypeName;
				auto rootType{ EFieldKind::Scalar };
				if( !column.PKTable ){
					if( column.Type==DB::EType::VarBinary || (prefix.size() && column.SKIndex) )//use NID see RolePermission's permissions, varbinary use case is passwords
						continue;
					fieldName = column.IsPK() ? "id" : ToJson( column.Name );
					qlTypeName = ColumnQL::QLType( column );//column.PKTable.empty() ? ColumnQL::QLType( column ) : dbTable.JsonName();
				}
				else{
					auto childColumn = dbTable.Map ? dbTable.Map->Child : nullptr;
					if( !isMap || column.PKTable->IsFlags || (childColumn && childColumn->Name==column.Name)  ){ //
						if( find_if(dbTable.Columns, [&column](let& c){return c->QLAppend==column.Name;})!=dbTable.Columns.end() )
							continue;
						if( mainTable.Extends && mainTable.Extends->GetPK()->Name==column.Name ){//extension table
							addColumns( *column.PKTable, false, prefix );
							continue;
						}
						qlTypeName = column.PKTable->JsonName();
						if( column.IsPK() ){ //roles
							fieldName = "id";
							qlTypeName = "ID";
						}else if( column.PKTable->IsFlags ){
							fieldName = ToPlural( ToJson(column.Name) );
							rootType = EFieldKind::List;
						}
						else if( childColumn ){
							fieldName = ToPlural( ToJson(Str::Replace(column.Name, "_id", "")) );
							rootType = EFieldKind::List;
						}
						else{
							fieldName = ToJson( qlTypeName );
							rootType = column.PKTable->IsEnum() ? EFieldKind::Enum : EFieldKind::Object;
						}
					}
					else{ //isMap
						//if( !typeName.starts_with(pPKTable->JsonName()) )//typeName==RolePermission, don't want role columns, just permissions.
						addColumns( *column.PKTable, false, column.PKTable->JsonName() );
						continue;
					}
				}

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
						let jsonType = pTable->Columns.size()==2 ? pTable2->JsonName() : pTable->JsonName();
						addField( ToPlural( ToJson(jsonType) ), {}, EFieldKind::List, jsonType, EFieldKind::Object );
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
		return jTable;
	}

α introspectEnum( const sp<DB::Table> baseTable, const TableQL& fieldTable)ε->jobject{
		auto dbTable = baseTable->QLView ? baseTable->QLView : AsView(baseTable);
		DB::SelectClause select;
		for_each( fieldTable.Columns, [&select, &dbTable](let& x){
			if( let c = x.JsonName=="id" ? dbTable->GetPK() : dbTable->FindColumn( x.JsonName ); c )
				select.TryAdd( {c} );
		});//sb only id/name.
		DB::Statement statement{
			select,
			{ {dbTable} },
			{},
			dbTable->GetPK()->Name
		};
		jarray fields;
		dbTable->Schema->DS()->Select( statement.Move(), [&]( DB::Row&& row ){
			jobject j;
			for( uint i=0; i<select.Columns.size(); ++i ){
				auto& c = *get<DB::AliasCol>(select.Columns[i]).Column;
				if( c.IsPK() )
					j["id"] = row.GetUInt( i );
				else
					j[c.Name] = move( row.GetString(i) );
			}
			fields.push_back( j );
		} );
		jobject jTable;
		jTable["enumValues"] = fields;
		return jTable;
	}

	α QueryType( const TableQL& typeTable )ε->jobject{
		let typeName = Json::AsString( typeTable.Args, "name" );
		auto dbTable = DB::AsTable( typeTable.DBTable );
		jobject y;
		for( let& qlTable : typeTable.Tables ){
			if( qlTable.JsonName=="fields" ){
				if( let pObject = _introspection.Find(typeName); pObject )
					y = pObject->ToJson( qlTable );
				else
					y = IntrospectFields( typeName, *dbTable, qlTable );
			}
			else if( qlTable.JsonName=="enumValues" )
				y = introspectEnum( dbTable, qlTable );
			else
				THROW( "__type data for '{}' not supported", qlTable.JsonName );
		}
		return y;
	}
	α QuerySchema( const TableQL& schemaTable )ε->jobject{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		let& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		jarray fields;
		for( let& schema : schemaTable.DBTable->Schema->DBSchema->AppSchemas ){
			for( let& nameTablePtr : schema.second->Tables ){
				let pDBTable = nameTablePtr.second;
				let childColumn = pDBTable->Map ? pDBTable->Map->Child : nullptr;
				let jsonType = pDBTable->JsonName();

				jobject field;
				field["name"] = Ƒ( "create{}"sv, jsonType );
				let addField = [&jsonType, pDBTable, &fields]( sv name, bool allColumns=false, bool idColumn=true ){
					jobject field;
					jarray args;
					for( let& column : pDBTable->Columns ){
						if( (column->IsPK() && !idColumn) || (!column->IsPK() && !allColumns) )
							continue;
						jobject arg;
						arg["name"] = ToJson( column->Name );
						arg["defaultValue"] = nullptr;
						jobject type; type["name"] = ColumnQL::QLType( *column );
						arg["type"]=type;
						args.push_back( arg );
					}
					field["args"] = args;
					field["name"] = Ƒ( "{}{}", name, jsonType );
					fields.push_back( field );
				};
				if( !childColumn ){
					addField( "insert", true, false );
					addField( "update", true );

					addField( "delete" );
					addField( "restore" );
					addField( "purge" );
				}
				else{
					addField( "add", true, false );
					addField( "remove", true, false );
				}
			}
		}
		jobject jmutationType;
		jmutationType["fields"] = fields;
		jmutationType["name"] = "Mutation";
		jobject jSchema; jSchema["mutationType"] = jmutationType;
		return jmutationType;
	}
}