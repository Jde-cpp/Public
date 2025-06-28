#include "MySqlServerMeta.h"
#include "MySqlDataSource.h"
#include "MySqlStatements.h"
#include <jde/db/Row.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include "../../src/meta/ddl/ColumnDdl.h"
#include "../../src/meta/ddl/ForeignKey.h"
#include "../../src/meta/ddl/Index.h"
#include "../../src/meta/ddl/Procedure.h"
#include "../../src/meta/ddl/TableDdl.h"

#define let const auto

namespace Jde::DB::MySql{
//	constexpr ELogTags _tags{ ELogTags::Sql };
	Ω loadTables( sp<IDataSource> ds, const MySqlServerMeta& meta, str schemaName, sv tablePrefix, bool like )ε->flat_map<string,sp<Table>>{
		flat_map<string,sp<Table>> tables;
		auto fromColumns = [&]( string&& tableName, string&& name, _int ordinalPosition, string&& dflt, string&& isNullable, sv type, optional<_int> maxLength, _int isIdentity, _int isId, optional<_int> numericPrecision, optional<_int> numericScale ){
			auto& table = tables.emplace( tableName, ms<TableDdl>(tableName) ).first->second;
			table->Columns.push_back( ms<ColumnDdl>(name, (uint)ordinalPosition, dflt, isNullable!="NO", meta.ToType(type), maxLength, isIdentity!=0, isId!=0, numericPrecision, numericScale) );
		};
		auto onRow = [&]( Row&& row ){
			fromColumns( move(row.GetString(0)), move(row.GetString(1)), move(row.GetInt(2)), move(row.GetString(3)), move(row.GetString(4)), move(row.GetString(5)), row.GetIntOpt(6), row.GetInt(7), row.GetInt(8), row.GetIntOpt(9), row.GetIntOpt(10) );
		};
		Sql sql{ Ddl::ColumnSql(tablePrefix), {Value{schemaName}} };
		if( tablePrefix.size() )
			sql.Params.emplace_back( like ? string{tablePrefix}+'%' : string{tablePrefix} );
		ds->Select( move(sql), onRow );
		let indexes = meta.LoadIndexes( tablePrefix );
		for( auto& index : indexes ){
			if( auto pTable = tables.find( index.TableName ); pTable!=tables.end() )
				std::dynamic_pointer_cast<TableDdl>(pTable->second)->Indexes.push_back( index );
		}
		return tables;
	}
	α MySqlServerMeta::LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>>{
		return loadTables( _pDataSource, *this, string{schemaName}, tablePrefix, true );
	}
	α MySqlServerMeta::LoadTable( str schemaName, str tableName, SL sl )Ε->sp<TableDdl>{
		auto tables = loadTables( _pDataSource, *this, string{schemaName}, tableName, false );
		THROW_IFSL( tables.empty(), "Table '{}' not found in schema '{}'.", tableName, schemaName );
		return std::dynamic_pointer_cast<TableDdl>(tables.begin()->second);
	}

	α MySqlServerMeta::LoadIndexes( sv tablePrefix, sv tableName )Ε->vector<Index>{
		let schema{ _pDataSource->SchemaName() };

		vector<Index> indexes;
		auto onRow = [&]( Row&& row ){
			uint i=0;
			auto tableName = row.GetString(i++);
			// if( tablePrefix.size() && tableName.size()>tablePrefix.size() )
			// 	tableName = tableName.substr( tablePrefix.size() );

			let indexName = row.GetString(i++); let columnName = row.GetString(i++); let unique = row.GetInt(i++)==0;
			vector<string>* pColumns;
			auto pExisting = find_if( indexes, [&](auto& index){ return index.Name==indexName && index.TableName==tableName; } );
			if( pExisting==indexes.end() ){
				bool clustered = false;//Boolean.Parse( row["CLUSTERED"].ToString() );
				bool primaryKey = indexName=="PRIMARY";//Boolean.Parse( row["PRIMARY_KEY"].ToString() );
				pColumns = &indexes.emplace_back( indexName, tableName, primaryKey, nullptr, unique, clustered ).Columns;
			}
			else
				pColumns = &pExisting->Columns;
			pColumns->push_back( columnName );
		};

		Sql sql{ Ddl::IndexSql(tablePrefix, tableName.size()), {Value{schema}} };
		if( tableName.size() )
			sql.Params.push_back( Value{string{tableName}} );
		else if( tablePrefix.size() )
			sql.Params.push_back( Value{string{tablePrefix}+'%'} );
		_pDataSource->Select( move(sql), onRow );

		return indexes;
	}

	α MySqlServerMeta::LoadProcs( str /*schemaName*/ )Ε->flat_map<string,Procedure>{
		let schema{ Value{_pDataSource->SchemaName()} };
		flat_map<string,Procedure> values;
		auto fnctn = [&values]( Row&& row ){
			string name = move( row.GetString(0) );
			values.try_emplace( name, Procedure{name} );
		};
		_pDataSource->Select( {Ddl::ProcSql(true), {schema}}, fnctn );
		return values;
	}

	α MySqlServerMeta::LoadForeignKeys( str /*schemaName*/ )Ε->flat_map<string,ForeignKey>{
		let schema{ _pDataSource->SchemaName() };
		flat_map<string,ForeignKey> fks;
		auto result = [&]( Row&& row ){
			uint i=0;
			let name = row.GetString(i++); let fkTable = row.GetString(i++); let column = row.GetString(i++); let pkTable = row.GetString(i++); //let pkColumn = row.GetString(i++); let ordinal = row.GetUInt(i);
			auto pExisting = fks.find( name );
			if( pExisting==fks.end() )
				fks.emplace( name, ForeignKey{name, fkTable, {column}, pkTable} );
			else
				pExisting->second.Columns.push_back( column );
		};
		_pDataSource->Select( {Ddl::ForeignKeySql(true), {Value{schema},Value{schema}}}, result );
		return fks;
	}

	α MySqlServerMeta::ToType( sv typeName )Ι->EType{
		using enum EType;
		auto type{ None };
		if( typeName=="datetime" || typeName=="timestamp" )
			type = DateTime;
		else if( typeName=="smalldatetime" )
			type = SmallDateTime;
		else if(typeName=="float")
			type = Float;
		else if(typeName=="real")
			type = SmallFloat;
		else if( typeName=="int" || typeName=="int(11)" )
			type = Int;
		else if( typeName=="int(10) unsigned" || typeName=="int unsigned" )
			type = UInt;
		else if( typeName=="bigint(21) unsigned" || typeName=="bigint(20) unsigned" )
			type = ULong;
		else if( Str::StartsWith(typeName, "bigint") )
			type = Long;
		else if( typeName=="nvarchar" )
			type = VarWChar;
		else if(typeName=="nchar")
			type = WChar;
		else if( Str::StartsWith(typeName, "smallint"))
			type = Int16;
		else if(typeName=="tinyint")
			type = Int8;
		else if( typeName=="tinyint unsigned" )
			type = UInt8;
		else if( typeName=="uniqueidentifier" )
			type = Guid;
		else if( typeName.starts_with("varbinary") )
			type = VarBinary;
		else if( Str::StartsWithInsensitive(typeName, "varchar") )
			type = VarChar;
		else if(typeName=="ntext")
			type = NText;
		else if(typeName=="text")
			type = Text;
		else if( typeName.starts_with("char") )
			type = Char;
		else if(typeName=="image")
			type = Image;
		else if( Str::StartsWith(typeName, "bit") )
			type = Bit;
		else if( Str::StartsWith(typeName, "binary") )
			type = Binary;
		else if( Str::StartsWith(typeName, "decimal") )
			type = Decimal;
		else if(typeName=="numeric")
			type = Numeric;
		else if(typeName=="money")
			type = Money;
		else
			Warning{ _tags, "Unknown datatype({}).  need to implement, ok if not our table.", typeName };
		return type;
	}
}