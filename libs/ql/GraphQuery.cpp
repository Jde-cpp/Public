#include "GraphQuery.h"
#include "GraphQL.h"
#include <jde/ql/GraphQLHook.h>
#include <jde/db/Database.h>
#include <jde/db/meta/Column.h>
#include <jde/db/names.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/SelectClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/generators/WhereClause.h>
#include "../../../Framework/source/DateTime.h"

#define let const auto

namespace Jde::QL{
	using std::endl;
	constexpr ELogTags _tags{ ELogTags::QL };
	using namespace DB::Names;
	α FindTable( str tableName )ε->sp<DB::View>;
	α GetEnumValues( const DB::View& table, SRCE )ε->sp<flat_map<uint,string>>{
		return table.Schema->DS()->SelectEnumSync<uint,string>( table, sl );
	}

	α numberToJson( const DB::Value& dbValue, const DB::Column& c, SRCE )ε->jvalue{
		jvalue y;
		if( c.Type==DB::EType::DateTime ){
			let v = dbValue.get_number<uint>();
			y = ToIsoString( TimePoint{std::chrono::seconds(v)} );
		}
		else if( c.PKTable && (c.IsEnum() || c.IsFlags()) ){
			let values = GetEnumValues( *c.PKTable );
			let value = dbValue.get_number<uint>();
			if( c.IsFlags() ){
				jarray flags;
				auto remainingFlags = value;
				for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 ){
					if( (remainingFlags & iFlag)==0 )
						continue;
					if( let flag = values->find(iFlag); flag!=values->end() )
						flags.emplace_back( flag->second );
					else
						flags.emplace_back( std::to_string(iFlag) );
					remainingFlags -= iFlag;
				}
				y = flags;
			}
			else //enum but not flags
				y = Find( *values, value ).value_or( std::to_string(value) );
		}
		else
			y = dbValue.ToJson();
		return y;
	}
	α ValueToJson( const DB::Value& dbValue, const ColumnQL* pMember=nullptr )ι->jvalue {
		using enum DB::EValue;
		jvalue json;
		switch( dbValue.Type() ){
			case UInt64: case Int32: case Int64: json = pMember && pMember->DBColumn ? numberToJson( dbValue, *pMember->DBColumn ) : dbValue.ToJson(); break;
			default: json = dbValue.ToJson();
		}
		return json;
	};

	α findFK( const DB::View& dbTable, string qlName )ε->sp<DB::Column>{
		auto fk = dbTable.FindColumn( qlName ); //members.
		if( !fk )
			fk = dbTable.FindColumn( ToSingular(qlName)+"_id" );//member_id
		return fk;
	}
	α findMap( const DB::View& dbTable, string qlName )ε->optional<DB::View::ParentChildMap>{
		optional<DB::View::ParentChildMap> map;
		if( auto mapTable = find_if(dbTable.Children, [&](auto& c){return c->Map->Child->PKTable->Name==qlName;}); mapTable!=dbTable.Children.end() ) //role_permissions
			map = (*mapTable)->Map;//permissionId
		return map;
	}

	α AddColumn( const ColumnQL& c, const TableQL& qlTable, const DB::View& dbTable, DB::Statement& statement, bool excludeId )ε->void{
		auto pk = dbTable.FindPK();
		let isPK = c.JsonName=="id";
		if( let table = isPK ? dynamic_cast<const DB::Table*>(&dbTable) : nullptr; table )
			pk = table->Extends ? table->Extends->GetPK() : pk;
	 	THROW_IF( isPK && !pk, "[{}]No id column.", qlTable.JsonName );

		auto columnName = isPK ? pk->Name : FromJson( c.JsonName );
		if( excludeId && isPK )
			return;
		auto dbColumn = isPK ? pk : dbTable.FindColumn( columnName );//want main table's pk, not extension's fk.  group maybe empty
		if( dbColumn && dbColumn->Table->Name!=dbTable.Name ){ //extension table.
			auto fk = findFK( dbTable, dbColumn->Table->Name );
			statement.From.TryAdd( {dbColumn->Table->GetPK(), fk, false} );//extension table & main table are joined on same name.
		}
		else if( !dbColumn ){
			if( let pEnum = dbTable.FindColumn(columnName+"_id"); pEnum ){//enumeration dbColumn==provider_id
				auto pFKTable = pEnum->PKTable->QLView
					? pEnum->PKTable->QLView //prvoiders_ql
					: pEnum->PKTable; //another enum
				statement.From.TryAdd( {pEnum, pFKTable->GetPK(), !pEnum->IsNullable} );
				dbColumn = pFKTable->GetColumnPtr( "name" );
			}
			THROW_IF( !dbColumn, "Could not find column '{}.{}'", dbTable.Name, columnName );
		}

		statement.Select.TryAdd( dbColumn );
		c.DBColumn = dbColumn;
		qlTable.JsonMembers.push_back( {qlTable.JsonName, c.JsonName} );
	}

	α ColumnSql( const TableQL& qlTable, const DB::View& dbTable, bool excludeId, DB::Statement& statement )->void{
		for( let& c : qlTable.Columns )
			AddColumn( c, qlTable, dbTable, statement, excludeId );

		statement.Where += QL::ToWhereClause( qlTable, dbTable, statement.Select.FindColumn("deleted")!=nullptr );
		for( let& qlChild : qlTable.Tables ){
			auto pFK = findFK( dbTable, qlChild.DBName() ); //members.
			if( !pFK ){
				if( auto pExtendedFromTable = AsTable(dbTable).Extends; pExtendedFromTable ){
					pFK = pExtendedFromTable->FindFK( pFK->PKTable->Name );
					if( pFK )
						statement.From.TryAdd( {pExtendedFromTable->GetPK(), dbTable.GetPK(), false} );
				}
			}
			if( pFK ){
				auto pkTable = pFK->PKTable;
				if( sp<DB::Table> table = AsTable( pkTable ); table && table->QLView )
					pkTable = table->QLView;
				statement.From.TryAdd( {pFK, pkTable->GetPK(), !pFK->IsNullable} );
				ColumnSql( qlChild, *pkTable, false, statement );
			}
			//else
			//	Error{ _tags, "Could not extract data {}->{}", dbTable.Name, qlChild.DBName() }; handle in subtables.
		}
	}
	//returns [qlTableName[parentTableId, jChildData]]
	α SelectSubTables( const vector<TableQL>& tables, const DB::Table& parentTable, DB::WhereClause where )->flat_map<string,flat_multimap<uint,jobject>>{
		flat_map<string,flat_multimap<uint,jobject>> subTables;
		for( auto& qlTable : tables ){//members
			auto fk = findFK( parentTable, qlTable.DBName() );
			DB::Statement statement;
			if( auto map = fk ? fk->Table->Map : nullopt; map ){ //identity_groups.member_id  if not a map, get it in main table.
				statement.Select.TryAdd( fk->Table->SurrogateKeys[0] );//add identity_id of groups for result.
				ColumnSql( qlTable, *fk->PKTable, false, statement );
				statement.From.TryAdd( {fk->PKTable->GetPK(), fk, true} ); //identities join identity_groups
			}
			else if( auto map = findMap(parentTable, qlTable.DBName()); map ){ //role_permissions
				auto parent = map->Parent; //role_id
				auto child = map->Child; //permission_id
				statement.Select.TryAdd( parent );
				ColumnSql( qlTable, *child->PKTable, false, statement ); //select id, allowed, denied
				statement.From.TryAdd( {parent->PKTable->GetPK(), parent, true} ); //from roles join role_permissions
				statement.From.TryAdd( {child, child->PKTable->GetPK(), true} ); //join permissions
			}
			else
				continue; //THROW_IF( !fk, "Could not find fk for {}->{}", parentTable.Name, qlTable.DBName() );

			statement.Where = where;
			auto& rows = subTables.emplace( qlTable.JsonName, flat_multimap<uint,jobject>{} ).first->second;
			auto forEachRow = [&]( const DB::IRow& row ){
				jobject jSubRow;
				uint index = 0;
				let rowToJson2 = [&row, &parentTable]( const vector<ColumnQL>& columns, bool checkId, jobject& jRow, uint& index2 ){
					int i = 1;//first should be pk of parent table.
					for( let& c : columns ){
						//auto i = checkId && c.DBColumn->IsPK() ? 1 : (index2++)+2;
/*						if( c.DBColumn->QLAppend.size() ){
							let pk = row[i++].ToUInt(); ++index2;
							let pColumn = c.DBColumn->Table->FindColumn( FromJson(c.DBColumn->QLAppend) );  CHECK( pColumn && pColumn->IsEnum() );
							let pEnum = parentTable.Schema->DS()->SelectEnumSync<uint,string>( pColumn->PKTable->Name ); CHECK( pEnum->find(pk)!=pEnum->end() );
							//jRow[c.JsonName] = ValueToJson( row, i, subFlagValues, &c );
							let name = Json::FindDefaultSV( jRow, c.JsonName );
							jRow[c.JsonName] = name.empty() ? pEnum->find(pk)->second : Ƒ( "{}\\{}", pEnum->find(pk)->second, name );
						}
						else*/
							jRow[c.JsonName] = ValueToJson( row[i++], &c );
					}
				};
				rowToJson2( qlTable.Columns, false, jSubRow, index );
				for( let& childTable : qlTable.Tables ){
					let childDbName = childTable.DBName();
					let pkTable = FindTable( childDbName );
					if( fk ){
						jobject jChildTable;
						rowToJson2( childTable.Columns, false, jChildTable, index );
						jSubRow[childTable.JsonName] = jChildTable;
					}
				}
				rows.emplace( row.GetUInt(0), jSubRow );
			};
			auto sql = statement.Move();
			parentTable.Schema->DS()->Select( move(sql.Text), forEachRow, move(sql.Params) );
		}
		return subTables;
	}
}
namespace Jde{
	α QL::SelectStatement( sp<DB::AppSchema> schema, const TableQL& qlTable, bool includeIdColumn/*, optional<DB::WhereClause> where*/ )ι->optional<DB::Statement>{
		let dbTable = schema->GetTablePtr( qlTable.DBName() );
		DB::Statement statement;
		ColumnSql( qlTable, *dbTable, false, statement );
		if( statement.Empty() )
			return {};
		if( statement.From.Empty() )
			statement.From.SingleTable = dbTable;

		if( string criteria = dbTable->Extends ? dbTable->SurrogateKeys[0]->Criteria : ""; criteria.size() ) //identities is_group
			statement.Where.Add( criteria );//group with no members.

		return statement;
	}

	α QL::Query( const TableQL& qlTable, jobject& jData, UserPK userPK )ε->void{
		optional<up<jobject>> hookData;
		[]( auto& qlTable, auto userPK, auto& jData, auto& hookData )->Task {
			AwaitResult result = co_await QL::Hook::Select( qlTable, userPK );
			hookData = result.UP<jobject>();
			if( *hookData )
				jData[qlTable.JsonName] = *(*hookData);
		}( qlTable, userPK, jData, hookData );
		while( !hookData )
			std::this_thread::yield();
		if( *hookData )
			return;
		let isPlural = qlTable.IsPlural();
		let dbTable = FindTable( qlTable.DBName() );
		DB::Statement statement = SelectStatement( dbTable->Schema, qlTable, true ).value();
/*		ColumnSql( qlTable, *dbTable, nullptr, flags, false, nullptr, statement, &jsonMembers );

		if( let addId = qlTable.Tables.size() && !qlTable.FindColumn("id") && qlTable.Columns.size(); addId ) //Why?
			statement.Select.TryAdd( dbTable->GetPK() );
		auto pExtendedFrom = AsTable(dbTable) ? AsTable(dbTable)->GetExtendedFromTable() : nullptr;
		if( statement.From.Empty() )
			statement.From.SingleTable = dbTable;
		vector<DB::Value> parameters;
		statement.Where = QL::ToWhereClause( qlTable, *dbTable );
		if( pExtendedFrom && dbTable->GetPK()->Criteria.size() ) //um_entities is_group
			statement.Where.Add( dbTable->GetPK()->Criteria );
*/
		let subTables = SelectSubTables( qlTable.Tables, *DB::AsTable(dbTable), statement.Where );
		let jsonTableName = qlTable.JsonName;
		auto addSubTables = [&]( jobject& jParent, uint id=0 ){
			for( let& qlTable : qlTable.Tables ){
				let subPlural = qlTable.JsonName.ends_with( "s" );
				if( subPlural )
					jParent[qlTable.JsonName] = jarray{};
				let pResultTable = subTables.find( qlTable.JsonName );
				if( pResultTable==subTables.end() )
					continue;
				let& subResults = pResultTable->second;
				if( !id && subResults.size() )
					id = subResults.begin()->first;
				auto range = subResults.equal_range( id );
				for( auto pRow = range.first; pRow!=range.second; ++pRow ){
					if( subPlural )
						jParent[qlTable.JsonName].get_array().emplace_back( pRow->second );
					else
						jParent[qlTable.JsonName] = pRow->second;
				}
			}
		};
		if( !statement.Empty() ){
			if( isPlural )
				jData[jsonTableName] = jarray{};
			auto rows = dbTable->Schema->DS()->Select( statement.Move() );
			for( let& row : rows ){
				auto jRow = qlTable.ToJson( *row, statement.Select.Columns );
				if( isPlural )
					jData[jsonTableName].get_array().emplace_back( move(jRow) );
				else
					jData[jsonTableName] = move(jRow);
				addSubTables( jRow, Json::FindNumber<uint>(jRow, "id").value_or(0) );
			}
		}
		else{
			jobject jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}
}