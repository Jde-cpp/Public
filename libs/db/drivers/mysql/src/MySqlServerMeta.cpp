#include "MySqlServerMeta.h"
#include <jde/db/IDataSource.h>//was MySqlDataSource.h, which drags in boost.mysql - nothing here needs the connection, and the test target compiles this file in.
#include "MySqlStatements.h"
#include <jde/db/Row.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include "../../../src/meta/ddl/ColumnDdl.h"
#include "../../../src/meta/ddl/ForeignKey.h"
#include "../../../src/meta/ServerMetaFold.h"
#include "../../../src/meta/ddl/Index.h"
#include "../../../src/meta/ddl/Procedure.h"
#include "../../../src/meta/ddl/TableDdl.h"

#define let const auto

namespace Jde::DB::MySql{
	Ω loadTables( IDataSource& ds, const MySqlServerMeta& meta, str schemaName, sv tablePrefix, bool like )ε->flat_map<string,sp<Table>>{
		flat_map<string,sp<Table>> tables;
		auto fromColumns = [&]( string&& tableName, string&& name, _int ordinalPosition, string&& dflt, string&& isNullable, sv type, optional<_int> maxLength, _int isIdentity, _int isId, optional<_int> numericPrecision, optional<_int> numericScale ){
			auto& table = tables.emplace( tableName, ms<TableDdl>(tableName) ).first->second;
			table->Columns.push_back( ms<ColumnDdl>(name, (uint)ordinalPosition, dflt, isNullable!="NO", meta.ToType(type), maxLength, isIdentity!=0, isId ? optional<uint8>{(uint8)(isId-1)} : optional<uint8>{}, numericPrecision, numericScale) );//isId is ColumnSql's hard-coded `0 is_id`, so this is nullopt today - but `isId!=0` made it an engaged optional{0}, i.e. every column claiming to be pk column 0.
		};
		auto onRow = [&]( Row&& row ){
			fromColumns( move(row.GetString(0)), move(row.GetString(1)), move(row.GetInt(2)), move(row.GetString(3)), move(row.GetString(4)), move(row.GetString(5)), row.GetIntOpt(6), row.GetInt(7), row.GetInt(8), row.GetIntOpt(9), row.GetIntOpt(10) );
		};
		let exact = !like && tablePrefix.size();//LoadTable wants an exact match - LIKE treats '_' in names like role_member as a wildcard.
		Sql sql{ Ddl::ColumnSql(tablePrefix, exact), {Value{schemaName}} };
		if( tablePrefix.size() )
			sql.Params.emplace_back( like ? string{tablePrefix}+'%' : string{tablePrefix} );
		ds.Select( move(sql), onRow );
		let indexes = meta.LoadIndexes( tablePrefix );
		for( auto& index : indexes ){
			if( auto pTable = tables.find( index.TableName ); pTable!=tables.end() )
				std::dynamic_pointer_cast<TableDdl>(pTable->second)->Indexes.push_back( index );
		}
		return tables;
	}
	α MySqlServerMeta::LoadTables( sv schemaName, sv tablePrefix )Ε->flat_map<string,sp<Table>>{
		return loadTables( _ds, *this, string{schemaName}, tablePrefix, true );
	}
	α MySqlServerMeta::LoadTable( str schemaName, str tableName, SL sl )Ε->sp<TableDdl>{
		auto tables = loadTables( _ds, *this, string{schemaName}, tableName, false );
		THROW_IFSL( tables.empty(), "Table '{}' not found in schema '{}'.", tableName, schemaName );
		return std::dynamic_pointer_cast<TableDdl>(tables.begin()->second);
	}

	α MySqlServerMeta::LoadIndexes( sv tablePrefix, sv tableName )Ε->vector<Index>{
		let schema{ _ds.SchemaName() };

		vector<Index> indexes;
		auto onRow = [&]( Row&& row ){
			uint i=0;
			auto tableName = row.GetString(i++);
			// if( tablePrefix.size() && tableName.size()>tablePrefix.size() )
			// 	tableName = tableName.substr( tablePrefix.size() );

			let indexName = row.GetString(i++); let columnName = row.GetString(i++); let unique = row.GetInt(i++)==0;
			FoldIndexRow( indexes, tableName, indexName, columnName, unique, indexName=="PRIMARY" );
		};

		Sql sql{ Ddl::IndexSql(tablePrefix, tableName.size()), {Value{schema}} };
		if( tableName.size() )
			sql.Params.push_back( Value{string{tableName}} );
		else if( tablePrefix.size() )
			sql.Params.push_back( Value{string{tablePrefix}+'%'} );
		_ds.Select( move(sql), onRow );

		return indexes;
	}

	α MySqlServerMeta::LoadProcs( str /*schemaName*/ )Ε->flat_map<string,Procedure>{
		let schema{ Value{_ds.SchemaName()} };
		flat_map<string,Procedure> values;
		auto fnctn = [&values]( Row&& row ){
			string name = move( row.GetString(0) );
			values.try_emplace( name, Procedure{name} );
		};
		_ds.Select( {Ddl::ProcSql(true), {schema}}, fnctn );
		return values;
	}

	α MySqlServerMeta::LoadForeignKeys( str /*schemaName*/ )Ε->flat_map<string,ForeignKey>{
		let schema{ _ds.SchemaName() };
		flat_map<string,ForeignKey> fks;
		auto result = [&]( Row&& row ){
			uint i=0;
			let name = row.GetString(i++); let fkTable = row.GetString(i++); let column = row.GetString(i++); let pkTable = row.GetString(i++); //let pkColumn = row.GetString(i++); let ordinal = row.GetUInt(i);
			FoldForeignKeyRow( fks, name, fkTable, column, pkTable );
		};
		_ds.Select( {Ddl::ForeignKeySql(true), {Value{schema},Value{schema}}}, result );
		return fks;
	}

	α ToDbType( sv columnType )ι->EType{
		using enum EType;
		let lower = Str::ToLower( columnType );
		const sv all{ lower };
		let isUnsigned = all.find( " unsigned" )!=sv::npos;
		let open = all.find( '(' );
		let base = all.substr( 0, std::min(open, all.find(' ')) );  //'double precision' -> 'double'; 'decimal(10,2)' -> 'decimal'.
		let width = open==sv::npos ? optional<uint>{} : Str::TryTo<uint>( string{all.substr(open+1, all.find_first_of(",)", open)-open-1)} );

		if( base=="datetime" || base=="timestamp" ) return DateTime;
		if( base=="date" || base=="smalldatetime" ) return SmallDateTime;
		if( base=="double" || base=="real" ) return Float;      //REAL is a DOUBLE synonym unless REAL_AS_FLOAT is set.
		if( base=="float" ) return SmallFloat;                  //#30: MySQL FLOAT is the 4-byte one.
		if( base=="bigint" ) return isUnsigned ? ULong : Long;
		if( base=="int" || base=="integer" || base=="mediumint" ) return isUnsigned ? UInt : Int;
		if( base=="smallint" ) return isUnsigned ? UInt16 : Int16;
		if( base=="tinyint" ) return width==1u ? Bit : isUnsigned ? UInt8 : Int8; //BOOL/BOOLEAN are aliases for tinyint(1).
		if( base=="bit" ) return Bit;                           //what this codebase's own DDL emits for Bit - see MySqlSyntax.
		if( base=="decimal" ) return Decimal;
		if( base=="numeric" ) return Numeric;
		if( base=="varchar" ) return VarChar;
		if( base=="char" ) return Char;
		if( base=="varbinary" ) return VarBinary;
		if( base=="binary" ) return Binary;                     //includes Guid columns: MySqlSyntax::GuidType() is binary(16).
		if( base=="text" || base=="tinytext" || base=="mediumtext" || base=="longtext" || base=="json" ) return Text;
		if( base=="blob" || base=="tinyblob" || base=="mediumblob" || base=="longblob" ) return Blob;
		//T-SQL spellings MySQL never returns; kept so nothing that fed this by hand changes answer.
		if( base=="nvarchar" ) return VarWChar;
		if( base=="nchar" ) return WChar;
		if( base=="ntext" ) return NText;
		if( base=="uniqueidentifier" ) return Guid;
		if( base=="image" ) return Image;
		if( base=="money" ) return Money;
		return None;
	}

	α MySqlServerMeta::ToType( sv typeName )Ι->EType{
		let type = ToDbType( typeName );//pure, so SyntaxTests covers the classification directly; the WARN stays here.
		if( type==EType::None )
			WARNT( ELogTags::Sql, "Unknown datatype({}).  need to implement, ok if not our table.", typeName );//WARNT, not WARN: `_tags` comes from the driver's usings.h, which this file no longer pulls in - see the include note above.
		return type;
	}
}