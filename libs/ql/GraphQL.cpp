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
	α QueryType( const TableQL& typeTable )ε->jobject;

	α QuerySchema( const TableQL& schemaTable )ε->jobject{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		let& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		jarray fields;
		for( let& schema : Schemas() ){
			for( let& nameTablePtr : schema->Tables ){
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

	α QueryTable( const TableQL& table, UserPK executer, bool log, SRCE )ε->jvalue{
		if( log )
			Trace{ sl, _tags, "{}.", table.ToString() };
		jvalue y;
		if( table.JsonName=="__type" )
			y = QueryType( table );
		else if( table.JsonName=="__schema" )
			y = QuerySchema( table );
		else{
			y = QL::Query( table, executer, sl );
		}
		return y;
	}

	α QueryTables( const vector<TableQL>& tables, UserPK userPK, bool log, SRCE )ε->jvalue{
		optional<jvalue> y;
		for( let& table : tables ){
			THROW_IFSL( table.Columns.empty() && table.Tables.empty(), "TableQL '{}' has no columns", table.ToString() );
			auto result = QueryTable( table, userPK, log, sl );
			if( table.ReturnRaw && tables.size()==1 )
				y = result;
			else{
				if( !y )
					y = jobject{};
				y->get_object()[table.JsonName] = move(result);
			}
		}
		return y ? *y : jvalue{};
	}
}}