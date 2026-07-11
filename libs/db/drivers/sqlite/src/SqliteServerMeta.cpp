#include <sqlite3.h>
#include "SqliteServerMeta.h"
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/meta/Table.h>
#include "../../../src/meta/ddl/ColumnDdl.h"
#include "../../../src/meta/ddl/ForeignKey.h"
#include "../../../src/meta/ddl/Index.h"
#include "../../../src/meta/ddl/Procedure.h"
#include "../../../src/meta/ddl/TableDdl.h"
#include "SqliteProcs.h"

#define let const auto

namespace Jde::DB::Sqlite{
	//'~' escape: '_' is a like wildcard and sqlite has no default escape char.
	constexpr sv userTableFilter{ "m.type='table' and m.name not like 'sqlite~_%' escape '~'" };

	//declared type 'varchar(255)' / 'decimal(10,2)' -> base name + length/precision/scale.
	Ω parseDeclaredType( sv declared )ι->std::tuple<string,optional<uint>,optional<uint>,optional<uint>>{
		let open = declared.find( '(' );
		string base{ Str::Trim(string{declared.substr(0, open)}) };
		optional<uint> length, precision, scale;
		if( open!=sv::npos ){
			let close = declared.find( ')', open );
			let args = declared.substr( open+1, close==sv::npos ? sv::npos : close-open-1 );
			let comma = args.find( ',' );
			if( let first = Str::TryTo<uint>(string{comma==sv::npos ? args : args.substr(0, comma)}); first ){
				length = precision = first;
				if( comma!=sv::npos )
					scale = Str::TryTo<uint>( string{args.substr(comma+1)} );
			}
		}
		return { move(base), length, precision, scale };
	}

	Ω loadTables( sp<IDataSource> ds, const SqliteServerMeta& meta, sv tableName, bool like )ε->flat_map<string,sp<Table>>{
		flat_map<string,sp<Table>> tables;
		auto onRow = [&]( Row&& row ){
			uint i=0;
			auto table = row.GetString( i++ );
			let name = row.GetString( i++ );
			let ordinal = row.GetUInt( i++ );
			let dflt = row.GetString( i++ );
			let isNullable = row.GetInt( i++ )==0;
			let declared = row.GetString( i++ );
			let pk = row.GetUInt( i++ );
			let pkCount = row.GetUInt( i++ );
			let [baseType, maxLength, precision, scale] = parseDeclaredType( declared );
			//rowid alias: only a single-column integer pk auto-assigns - see SqliteSyntax::ToString/CreatePrimaryKey.
			let isIdentity = pk==1 && pkCount==1 && Str::ToLower(baseType).find("int")!=string::npos;
			auto& pTable = tables.emplace( table, ms<TableDdl>(table) ).first->second;
			pTable->Columns.push_back( ms<ColumnDdl>(name, ordinal, dflt, isNullable, meta.ToType(baseType), maxLength, isIdentity, pk ? optional<uint8>((uint8)(pk-1)) : optional<uint8>{}, precision, scale) );
		};
		Sql sql{ Ƒ("select m.name, ti.name, ti.cid, coalesce(ti.dflt_value,''), ti.\"notnull\", ti.type, ti.pk,"
			"\n\t(select count(*) from pragma_table_info(m.name) p where p.pk>0)"
			"\nfrom sqlite_master m, pragma_table_info(m.name) ti"
			"\nwhere {}{}"
			"\norder by m.name, ti.cid", userTableFilter, tableName.size() ? (like ? " and m.name like ?" : " and m.name=?") : "") };
		if( tableName.size() )
			sql.Params.emplace_back( like ? string{tableName}+'%' : string{tableName} );
		ds->Select( move(sql), onRow );
		let indexes = meta.LoadIndexes( like ? tableName : sv{}, like ? sv{} : tableName );
		for( auto& index : indexes ){
			if( auto pTable = tables.find( index.TableName ); pTable!=tables.end() )
				std::dynamic_pointer_cast<TableDdl>( pTable->second )->Indexes.push_back( index );
		}
		return tables;
	}

	α SqliteServerMeta::LoadTables( sv /*schemaName - always 'main'*/, sv tablePrefix )Ε->flat_map<string,sp<Table>>{
		return loadTables( _pDataSource, *this, tablePrefix, true );
	}
	α SqliteServerMeta::LoadTable( str /*schemaName*/, str tableName, SL sl )Ε->sp<TableDdl>{
		auto tables = loadTables( _pDataSource, *this, tableName, false );
		THROW_IFSL( tables.empty(), "Table '{}' not found.", tableName );
		return std::dynamic_pointer_cast<TableDdl>( tables.begin()->second );
	}

	α SqliteServerMeta::LoadIndexes( sv tablePrefix, sv tableName )Ε->vector<Index>{
		vector<Index> indexes;
		auto onRow = [&indexes]( Row&& row ){
			uint i=0;
			let table = row.GetString( i++ );
			let indexName = row.GetString( i++ );
			let columnName = row.GetString( i++ ); //empty for rowid/expression members.
			let unique = row.GetInt( i++ )==1;
			let primaryKey = row.GetString( i++ )=="pk"; //origin: 'c'=create index, 'u'=unique constraint, 'pk'.
			auto pExisting = find_if( indexes, [&](auto& index){ return index.Name==indexName && index.TableName==table; } );
			auto& columns = pExisting==indexes.end()
				? indexes.emplace_back( indexName, table, primaryKey, nullptr, unique, optional<bool>{} ).Columns
				: pExisting->Columns;
			columns.push_back( columnName );
		};
		string filter;
		Sql sql;
		if( tableName.size() ){
			filter = " and m.name=?";
			sql.Params.emplace_back( string{tableName} );
		}
		else if( tablePrefix.size() ){
			filter = " and m.name like ?";
			sql.Params.emplace_back( string{tablePrefix}+'%' );
		}
		sql.Text = Ƒ( "select m.name, il.name, coalesce(ii.name,''), il.\"unique\", il.origin"
			"\nfrom sqlite_master m, pragma_index_list(m.name) il, pragma_index_info(il.name) ii"
			"\nwhere {}{}"
			"\norder by m.name, il.seq, ii.seqno", userTableFilter, filter );
		_pDataSource->Select( move(sql), onRow );
		return indexes;
	}

	α SqliteServerMeta::LoadForeignKeys( str /*schemaName*/ )Ε->flat_map<string,ForeignKey>{
		flat_map<string,ForeignKey> fks;
		string current; //pragma doesn't expose constraint names - synthesize '<table>_fk<id>'; SyncFKs matches on Table+Columns, not name.
		auto onRow = [&]( Row&& row ){
			uint i=0;
			let table = row.GetString( i++ );
			let id = row.GetUInt( i++ );
			let pkTable = row.GetString( i++ );
			let column = row.GetString( i++ );
			let name = Ƒ( "{}_fk{}", table, id );
			auto pExisting = fks.find( name );
			if( pExisting==fks.end() )
				fks.emplace( name, ForeignKey{name, table, {column}, pkTable} );
			else
				pExisting->second.Columns.push_back( column );
		};
		_pDataSource->Select( {Ƒ("select m.name, fk.id, fk.\"table\", fk.\"from\""
			"\nfrom sqlite_master m, pragma_foreign_key_list(m.name) fk"
			"\nwhere {}"
			"\norder by m.name, fk.id, fk.seq", userTableFilter)}, onRow );
		return fks;
	}

	α SqliteServerMeta::LoadProcs( str /*schemaName*/ )Ε->flat_map<string,Procedure>{
		//No server procs - report the native registry so DDL sync treats registered procs as existing.
		flat_map<string,Procedure> procs;
		for( auto& name : RegisteredProcNames() ){
			let key = name;
			procs.try_emplace( key, Procedure{move(name)} );
		}
		return procs;
	}

	α SqliteServerMeta::ToType( sv name )Ι->EType{
		using enum EType;
		let [base, length, precision, scale] = parseDeclaredType( name );
		let typeName = Str::ToLower( base );
		auto type{ None };
		if( typeName=="integer" || typeName=="bigint" || typeName=="int8" )
			type = Long;
		else if( typeName=="int" || typeName=="mediumint" )
			type = Int;
		else if( typeName=="int unsigned" || typeName=="integer unsigned" )
			type = UInt;
		else if( typeName=="bigint unsigned" )
			type = ULong;
		else if( typeName=="smallint" || typeName=="int2" )
			type = Int16;
		else if( typeName=="tinyint unsigned" )
			type = UInt8;
		else if( typeName=="tinyint" )
			type = Int8;
		else if( typeName=="datetime" || typeName=="timestamp" )
			type = DateTime;
		else if( typeName=="smalldatetime" )
			type = SmallDateTime;
		else if( typeName=="float" || typeName=="double" || typeName=="double precision" )
			type = Float;
		else if( typeName=="real" )
			type = SmallFloat;
		else if( typeName=="blob" || typeName=="uniqueidentifier" || typeName=="guid" )
			type = Guid; //bare 'blob' is our GuidType - see SqliteSyntax; bytes columns declare varbinary/binary/image.
		else if( typeName=="varbinary" )
			type = VarBinary;
		else if( typeName=="binary" )
			type = Binary;
		else if( typeName=="image" )
			type = Image;
		else if( typeName=="nvarchar" )
			type = VarWChar;
		else if( typeName=="nchar" )
			type = WChar;
		else if( typeName=="varchar" )
			type = VarChar;
		else if( typeName=="char" || typeName=="character" )
			type = Char;
		else if( typeName=="ntext" )
			type = NText;
		else if( typeName=="text" || typeName=="clob" )
			type = Text;
		else if( typeName=="bit" || typeName=="bool" || typeName=="boolean" )
			type = Bit;
		else if( typeName=="decimal" )
			type = Decimal;
		else if( typeName=="numeric" )
			type = Numeric;
		else if( typeName=="money" )
			type = Money;
		else{ //sqlite affinity rules for types created outside our ddl: https://sqlite.org/datatype3.html#determination_of_column_affinity
			if( typeName.find("int")!=string::npos )
				type = Long;
			else if( typeName.find("char")!=string::npos || typeName.find("clob")!=string::npos || typeName.find("text")!=string::npos )
				type = VarChar;
			else if( typeName.empty() || typeName.find("blob")!=string::npos )
				type = VarBinary;
			else if( typeName.find("real")!=string::npos || typeName.find("floa")!=string::npos || typeName.find("doub")!=string::npos )
				type = Float;
			else
				type = Numeric;
			WARN( "Unmapped declared type '{}' - using affinity fallback {}.", name, (uint)type );
		}
		return type;
	}
}
