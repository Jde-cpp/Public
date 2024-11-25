#include <jde/framework/str.h>
#include <jde/framework/io/json.h>
//#include <jde/ql/GraphQLHook.h>
//#include <jde/framework/coroutine/TaskOld.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/Introspection.h>
#include <jde/db/IRow.h>
#include <jde/db/Database.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/Syntax.h>
//#include <jde/db/generators/UpdateStatement.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#include "../../../Framework/source/DateTime.h"
#include "GraphQuery.h"
//#include "types/QLColumn.h"
//#include "types/Parser.h"

#define let const auto
namespace Jde{
	using DB::EValue;
	using DB::EType;
	using namespace DB::Names;
//	using QL::EFieldKind;
	constexpr ELogTags _tags{ ELogTags::QL };

namespace QL{
	α Schemas()ι->const vector<sp<DB::AppSchema>>&;
	α QueryType( const TableQL& typeTable, jobject& jData )ε->void;

	α QuerySchema( const TableQL& schemaTable, jobject& jData )ε->void{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		let& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		jarray fields;
		for( let& schema : Schemas() ){
			for( let& nameTablePtr : schema->Tables ){
				let pDBTable = nameTablePtr.second;
				let childColumn = pDBTable->Map ? pDBTable->Map->Child : nullptr;
				let jsonType = pDBTable->JsonTypeName();

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
		jData["__schema"] = jmutationType;
	}

	α QueryTable( const TableQL& table, UserPK userPK, jobject& jData )ε->void{
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
		return data;
	}
}}