#include "MsSqlSchemaProc.h"
#include <jde/fwk/Str.h>
#include <jde/db/Row.h>
#include <jde/db/meta/Table.h>
#include "../../../src/meta/ddl/ForeignKey.h"
#include "../../../src/meta/ddl/Index.h"
#include "../../../src/meta/ddl/Procedure.h"
#include "../../../src/meta/ddl/TableDdl.h"
#include "MsSqlStatements.h"
#include "../OdbcDataSource.h"
#define let const auto

namespace Jde::DB::MsSql{
		α processRow( flat_map<string,sp<Table>>& tables, str tableName, sv name, _int /*ordinalPosition*/, sv dflt, bool isNullable, sv type, optional<_int> maxLength, _int isIdentity, _int /*isId*/, optional<_int> numericPrecision, optional<_int> numericScale )->void{
			auto& table = tables.emplace( tableName, ms<TableDdl>(Table{tableName}) ).first->second;
			let dataType = ToType(type);
			optional<Value> defaultValue;
			if( !dflt.empty() ){
				if( dataType==EType::Int ){
					auto start = dflt.find_first_of( '\'' );
					auto end = dflt.find_last_of( '\'' );
					if( end-start<2 && dflt.size()>4 ){//"((?))
						start = 1;
						end = dflt.size()-4;
					}
					if( let value = end-start>1 ? Str::TryTo<_int>(string{dflt.substr(start+1, end-start)}) : std::optional<_int>{}; value )
						defaultValue = *value;
				}
				else if( dataType==EType::Bit )
					defaultValue = dflt!="((0))";
			}
			auto c = ms<Column>( string{name} );
			c->Default = defaultValue;
			c->IsNullable = isNullable;
			c->IsSequence = isIdentity != 0;
			c->Type = dataType;
			c->MaxLength = maxLength.value_or(0);
			c->NumericPrecision = numericPrecision.value_or(0);
			c->NumericScale = numericScale.value_or(0);
			c->Table = table;
			table->Columns.push_back( c );
		}

	α MsSqlSchemaProc::LoadColumns( DB::Sql&& sql )Ε->flat_map<string,sp<Table>>{
		auto rows = _pDataSource->Select( move(sql) );
		flat_map<string,sp<Table>> tables;
		for( auto&& row : rows ){
			processRow( tables, move(row.GetString(0)), move(row.GetString(1)), row.GetInt(2), move(row.GetString(3)), row.GetBit(4), move(row.GetString(5)), row.GetIntOpt(6), row.GetInt(7), row.GetInt(8), row.GetIntOpt(9), row.GetIntOpt(10) );
		}
		return tables;
	}

	α MsSqlSchemaProc::LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>>{
		DB::Sql sql{ Sql::ColumnSql(tablePrefix.size()), {Value{string{schemaName}}} };
		if (tablePrefix.size())
			sql.Params.push_back( Value{string{tablePrefix}+'%'} );

		auto tables = LoadColumns( move(sql) );
		let indexes = LoadIndexes( schemaName, {} );
		for( auto& index : indexes ){
			if( auto table = tables.find(index.TableName); table != tables.end() )
				std::dynamic_pointer_cast<TableDdl>( table->second )->Indexes.push_back( index );
		}
		return tables;
	}
	α MsSqlSchemaProc::LoadTable( str schemaName, str tableName, SL sl )Ε->sp<TableDdl>{
		auto sql = DB::Sql{ Sql::ColumnSql(true), {Value{schemaName}, Value{tableName}} };
		auto tables = LoadColumns( move(sql) );
		THROW_IFSL( tables.size()!=1, "Table not found '{}.{}'. size={}", schemaName, tableName, tables.size() );
		auto dbTable = dynamic_pointer_cast<TableDdl>( tables.begin()->second );
		dbTable->Indexes = LoadIndexes( schemaName, dbTable->Name );
		return dbTable;
	}

	α MsSqlSchemaProc::LoadIndexes( sv schema, sv tableName )Ε->vector<Index>{
		if( schema.empty() )
			schema = "dbo";// _pDataSource->Catalog( MsSql::Sql::CatalogSql );

		vector<Index> indexes;
		auto result = [&]( Row&& row ){
			uint i=0;
			let tableName = move(row.GetString(i++)); let indexName = move(row.GetString(i++)); let columnName = move(row.GetString(i++)); let unique = row.GetBit(i++)==0;

			vector<string>* pColumns;
			auto pExisting = std::find_if( indexes.begin(), indexes.end(), [&](auto index){ return index.Name==indexName && index.TableName==tableName; } );
			if( pExisting==indexes.end() ){
				bool clustered = false;//Boolean.Parse( row["CLUSTERED"].ToString() );
				let primaryKey = indexName==Ƒ( "{}_pk", tableName );//Boolean.Parse( row["PRIMARY_KEY"].ToString() );
				pColumns = &indexes.emplace_back( indexName, tableName, primaryKey, nullptr, unique, clustered ).Columns;
			}
			else
				pColumns = &pExisting->Columns;
			pColumns->push_back( columnName );
		};

		vector<Value> values{ Value{string{schema}} };
		if( tableName.size() )
			values.push_back( Value{string{tableName}} );
		let sql = Sql::IndexSql( tableName.size() );
		_pDataSource->Select( {sql, values}, result );

		return indexes;
	}

	α MsSqlSchemaProc::LoadProcs( str schemaName )Ε->flat_map<string,Procedure>{
		flat_map<string,Procedure> values;
		auto fnctn = [&]( Row&& row ){
			let name = move(row.GetString( 0 ));
			values.try_emplace( name, Procedure{name, schemaName} );
		};
		_pDataSource->Select( {Sql::ProcSql(true), {Value{schemaName}}}, fnctn );
		return values;
	}

	α MsSqlSchemaProc::ToType( sv typeName )Ι->EType{
		EType type{ EType::None };
		using enum EType;
		if(typeName=="datetime")
			type=DateTime;
		else if( typeName=="smalldatetime" )
			type=SmallDateTime;
		else if(typeName=="float")
			type=Float;
		else if(typeName=="real")
			type=SmallFloat;
		else if( typeName=="int" )
			type = Int;
		else if( Str::StartsWith(typeName, "bigint") )
			type=Long;
		else if( typeName=="nvarchar" || typeName=="sysname" )
			type=VarWChar;
		else if(typeName=="nchar")
			type=WChar;
		else if( Str::StartsWith(typeName, "smallint") )
			type=Int16;
		else if(typeName=="tinyint")
			type=Int8;
		else if( typeName=="tinyint unsigned" )
			type=UInt8;
		else if( typeName=="uniqueidentifier" )
			type=Guid;
		else if(typeName=="varbinary")
			type=VarBinary;
		else if( Str::StartsWithInsensitive(typeName, "varchar") )
			type=VarChar;
		else if(typeName=="ntext")
			type=NText;
		else if(typeName=="text")
			type=Text;
		else if(typeName=="char")
			type=Char;
		else if(typeName=="image")
			type=Image;
		else if(Str::StartsWith(typeName, "bit") )
			type=Bit;
		else if( Str::StartsWith(typeName, "binary") )
			type=Binary;
		else if( Str::StartsWith(typeName, "decimal") )
			type=Decimal;
		else if(typeName=="numeric")
			type=Numeric;
		else if(typeName=="money")
			type=Money;
		else
			WARNT( ELogTags::App, "Unknown datatype({}).  need to implement, no big deal if not our table.", typeName );
		return type;
	}

	α MsSqlSchemaProc::LoadForeignKeys( str schemaName )Ε->flat_map<string,ForeignKey>{
		flat_map<string,ForeignKey> fks;
		auto result = [&]( Row&& row ){
			uint i=0;
			let name = move(row.GetString(i++)); let fkTable = move(row.GetString(i++)); let column = move(row.GetString(i++)); let pkTable = move(row.GetString(i++)); //let pkColumn = row.GetString(i++); let ordinal = row.GetUInt(i);
			auto pExisting = fks.find( name );
			if( pExisting==fks.end() )
				fks.emplace( name, ForeignKey{name, fkTable, {column}, pkTable} );
			else
				pExisting->second.Columns.push_back( column );
		};
		_pDataSource->Select( {Sql::ForeignKeySql(schemaName.size()), {Value{schemaName}}}, result );
		return fks;
	}
}