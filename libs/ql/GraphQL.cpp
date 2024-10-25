#include "GraphQL.h"
#include <jde/framework/str.h>
#include <jde/framework/io/json.h>
#include <jde/ql/GraphQLHook.h>
//#include <jde/framework/coroutine/TaskOld.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/Introspection.h>
#include <jde/db/IRow.h>
#include <jde/db/Database.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/generators/UpdateStatement.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#include "../../../Framework/source/DateTime.h"
#include "GraphQuery.h"
//#include "types/QLColumn.h"
#include "types/Parser.h"

#define let const auto
namespace Jde{
	using DB::EValue;
	using DB::EType;
	using namespace DB::Names;
//	using QL::EFieldKind;
	constexpr ELogTags _tags{ ELogTags::QL };

namespace QL{
	Introspection _introspection;
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	α FindTable( str tableName )ε->sp<DB::View>;
	α IntrospectFields( sv /*typeName*/, const DB::Table& mainTable, const TableQL& fieldTable, jobject& jData )ε->void;
	α IntrospectEnum( sv /*typeName*/, const DB::Table& baseTable, const TableQL& fieldTable, jobject& jData )ε->void;

/*	α SubWhere( const DB::Table& table, const DB::Column& c, vector<DB::Value>& params, uint paramIndex )ε->string{
		std::ostringstream sql{ "=( select id from ", std::ios::ate }; sql << table.Name << " where " << c.Name;
		if( c.QLAppend.size() ){
			CHECK( paramIndex<params.size() && params[paramIndex].index()==(uint)EValue::String );
			let split = Str::Split( get<string>(params.back()), "\\" ); CHECK( split.size() );
			let appendColumnName = c.QLAppend;
			let pColumn = table.FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable );
			sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
			if( split.size()>1 ){
				params.push_back( string{split[1]} );
				params[paramIndex] = string{ split[0] };
			}
		}
		else
			sql << "=? )";
		return sql.str();
	}*/


	α QueryType( const TableQL& typeTable, jobject& jData )ε->void{
		let typeName = Json::AsString( typeTable.Args, "name" );
		auto dbTable = DB::AsTable( FindTable(ToPlural(FromJson(typeName))) );
		for( let& qlTable : typeTable.Tables ){
			if( qlTable.JsonName=="fields" ){
				if( let pObject = _introspection.Find(typeName); pObject )
					jData["__type"] = pObject->ToJson( qlTable );
				else
					IntrospectFields( typeName, *dbTable, qlTable, jData );
			}
			else if( qlTable.JsonName=="enumValues" )
				IntrospectEnum( typeName, *dbTable, qlTable, jData );
			else
				THROW( "__type data for '{}' not supported", qlTable.JsonName );
		}
	}
	α QuerySchema( const TableQL& schemaTable, jobject& jData )ε->void{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		let& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		jarray fields;
		for( let& schema : Schemas() ){
			for( let& nameTablePtr : schema->Tables ){
				let pDBTable = nameTablePtr.second;
				let childColumn = pDBTable->ChildColumn();
				let jsonType = pDBTable->JsonTypeName();

				jobject field;
				field["name"] = Ƒ( "create{}"sv, jsonType );
				let addField = [&jsonType, pDBTable, &fields]( sv name, bool allColumns=false, bool idColumn=true ){
					jobject field;
					jarray args;
					for( let& column : pDBTable->Columns ){
						if(   (column->IsPK() && !idColumn) || (!column->IsPK() && !allColumns) )
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
		jData["__schema"] = jmutationType;
	}
#define TEST_ACCESS(a,b,c) Trace( _tags, "TEST_ACCESS({},{},{})", a, b, c )
	α QueryTable( const TableQL& table, UserPK userPK, jobject& jData )ε->void{
		TEST_ACCESS( "Read", table.DBName(), userPK ); //TODO implement.
		if( table.JsonName=="__type" )
			QueryType( table, jData );
		else if( table.JsonName=="__schema" )
			QuerySchema( table, jData );
		else
			QL::Query( table, jData, userPK );
	}

	α QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->jobject{
		jobject data;
		for( let& table : tables )
			QueryTable( table, userPK, data );

		jobject y;
		y["data"] = data;
		return y;
	}
}}