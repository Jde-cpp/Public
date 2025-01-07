#include "MySqlServerMeta.h"
#include "MySqlDataSource.h"
#include "MySqlStatements.h"
#include <jde/db/IRow.h>
#include <jde/db/generators/Syntax.h>
#include "../../meta/ddl/ColumnDdl.h"
#include "../../meta/ddl/ForeignKey.h"
#include "../../meta/ddl/Index.h"
#include "../../meta/ddl/Procedure.h"
#include "../../meta/ddl/TableDdl.h"

#define let const auto

namespace Jde::DB::MySql{
	constexpr ELogTags _tags{ ELogTags::Sql };

	α MySqlServerMeta::LoadTables( sv tablePrefix )Ε->flat_map<string,sp<Table>>{
		let schema{ _pDataSource->SchemaName() };
		flat_map<string,sp<Table>> tables;
		auto fromColumns = [&]( sv tableName, sv name, _int ordinalPosition, sv dflt, string isNullable, sv type, optional<_int> maxLength, _int isIdentity, _int isId, optional<_int> numericPrecision, optional<_int> numericScale ){
			if( tablePrefix.size() && tableName.size()>tablePrefix.size() )
				tableName = tableName.substr( tablePrefix.size() );
			auto& table = tables.emplace( tableName, ms<TableDdl>(tableName) ).first->second;
			table->Columns.push_back( ms<ColumnDdl>(name, (uint)ordinalPosition, dflt, isNullable!="NO", ToType(type), maxLength, isIdentity!=0, isId!=0, numericPrecision, numericScale) );
		};
		auto onRow = [&]( IRow& row ){
			fromColumns( row.MoveString(0), row.MoveString(1), row.GetInt(2), row.MoveString(3), row.MoveString(4), row.MoveString(5), row.GetIntOpt(6), row.GetInt(7), row.GetInt(8), row.GetIntOpt(9), row.GetIntOpt(10) );
		};
		let sql = Sql::ColumnSql( tablePrefix );
		vector<Value> params{ Value{schema} };
		if( tablePrefix.size() )
			params.emplace_back( string{tablePrefix}+'%' );
		_pDataSource->Select( sql, onRow, params );
		let indexes = LoadIndexes( tablePrefix );
		for( auto& index : indexes ){
			if( auto pTable = tables.find( index.TableName ); pTable!=tables.end() )
				std::dynamic_pointer_cast<TableDdl>(pTable->second)->Indexes.push_back( index );
		}
		return tables;
	}

	α MySqlServerMeta::LoadIndexes( sv tablePrefix, sv tableName )Ε->vector<Index>{
		let schema{ _pDataSource->SchemaName() };

		vector<Index> indexes;
		auto onRow = [&]( IRow& row ){
			uint i=0;
			auto tableName = row.MoveString(i++);
			if( tablePrefix.size() && tableName.size()>tablePrefix.size() )
				tableName = tableName.substr( tablePrefix.size() );

			let indexName = row.MoveString(i++); let columnName = row.MoveString(i++); let unique = row.GetBit(i++)==0;
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

		vector<Value> values{Value{schema}};
		if( tableName.size() )
			values.push_back( Value{string{tableName}} );
		else if( tablePrefix.size() )
			values.push_back( Value{string{tablePrefix}+'%'} );
		_pDataSource->Select( Sql::IndexSql(tablePrefix, tableName.size()), onRow, values );

		return indexes;
	}

	α MySqlServerMeta::LoadProcs()Ε->flat_map<string,Procedure>{
		let schema{ _pDataSource->SchemaName() };
		flat_map<string,Procedure> values;
		auto fnctn = [&values]( IRow& row ){
			string name = row.MoveString(0);
			values.try_emplace( name, Procedure{name} );
		};
		_pDataSource->Select( Sql::ProcSql(true), fnctn, {Value{schema}} );
		return values;
	}

	α MySqlServerMeta::LoadForeignKeys()Ε->flat_map<string,ForeignKey>{
		let schema{ _pDataSource->SchemaName() };
		flat_map<string,ForeignKey> fks;
		auto result = [&]( IRow& row ){
			uint i=0;
			let name = row.MoveString(i++); let fkTable = row.MoveString(i++); let column = row.MoveString(i++); let pkTable = row.MoveString(i++); //let pkColumn = row.GetString(i++); let ordinal = row.GetUInt(i);
			auto pExisting = fks.find( name );
			if( pExisting==fks.end() )
				fks.emplace( name, ForeignKey{name, fkTable, {column}, pkTable} );
			else
				pExisting->second.Columns.push_back( column );
		};
		_pDataSource->Select( Sql::ForeignKeySql(true), result, {Value{schema},Value{schema}} );
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