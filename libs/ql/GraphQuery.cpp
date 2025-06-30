#include <jde/ql/ql.h>
#include <jde/framework/chrono.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/FromClause.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/SelectClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/generators/WhereClause.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/View.h>
#include <jde/ql/QLHook.h>

#define let const auto

namespace Jde::QL{
	using std::endl;
	constexpr ELogTags _tags{ ELogTags::QL };
	using namespace DB::Names;
	α GetTable( str tableName, SRCE )ε->sp<DB::View>;
	α GetEnumValues( const DB::View& table, SRCE )ε->flat_map<uint,string>{
		flat_map<uint,string> foo;
		return table.Schema->DS()->SelectEnumSync<uint,string>( table, sl );
	}
	using SubTables=flat_map<string,flat_multimap<uint,jobject>>;//tableName,pk, row
	Ω addSubTables( const TableQL& parentQL, const SubTables& subTables, jobject& parent, uint parentId )ι->void;
	α numberToJson( const DB::Value& dbValue, const DB::Column& c, SRCE )ε->jvalue{
		jvalue y;
		if( c.Type==DB::EType::DateTime ){
			let v = dbValue.get_number<uint>();
			y = ToIsoString( TimePoint{std::chrono::seconds(v)} );
		}
		else if( c.PKTable && (c.IsEnum() || c.IsFlags()) ){
			let values = GetEnumValues( *c.PKTable );
			let value = dbValue.get_number<uint>( sl );
			if( c.IsFlags() ){
				jarray flags;
				auto remainingFlags = value;
				for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 ){
					if( (remainingFlags & iFlag)==0 )
						continue;
					if( let flag = values.find(iFlag); flag!=values.end() )
						flags.emplace_back( flag->second );
					else
						flags.emplace_back( std::to_string(iFlag) );
					remainingFlags -= iFlag;
				}
				y = flags;
			}
			else //enum but not flags
				y = Find( values, value ).value_or( std::to_string(value) );
		}
		else if( c.Type==DB::EType::Bit )
			y = dbValue.ToUInt()!=0;
		else
			y = dbValue.ToJson();
		return y;
	}
	α ValueToJson( DB::Value&& dbValue, const ColumnQL* pMember=nullptr )ι->jvalue {
		using enum DB::EValue;
		jvalue json;
		switch( dbValue.Type() ){
			case UInt64: case Int32: case Int64: json = pMember && pMember->DBColumn ? numberToJson( dbValue, *pMember->DBColumn ) : dbValue.ToJson(); break;
			default: json = dbValue.Move();
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
		if( auto mapTable = find_if(dbTable.Children, [&](auto& c){return c->Map->Child->PKTable->Name==qlName;}); mapTable!=dbTable.Children.end() ) //role_members
			map = (*mapTable)->Map;//permissionId
		return map;
	}

	Ω addColumn( const ColumnQL& c, const TableQL& qlTable, const DB::View& dbTable, DB::Statement& statement, bool excludeId )ε->void{
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

	Ω columnSql( const TableQL& qlTable, const DB::View& dbTable, bool excludeId, DB::Statement& statement, optional<bool> includeDeleted=nullopt )ε->void{
		for( let& c : qlTable.Columns )
			addColumn( c, qlTable, dbTable, statement, excludeId );

		statement.Where += QL::ToWhereClause( qlTable, dbTable, includeDeleted.value_or(statement.Select.FindColumn("deleted")!=nullptr) );
		for( let& qlChild : qlTable.Tables ){
			auto pFK = findFK( dbTable, qlChild.DBName() ); //members.
			if( pFK ){
				auto pkTable = pFK->PKTable;
				if( sp<DB::Table> table = AsTable( pkTable ); table && table->QLView )
					pkTable = table->QLView;
				statement.From.TryAdd( {pFK, pkTable->GetPK(), !pFK->IsNullable} );
				columnSql( qlChild, *pkTable, false, statement, includeDeleted );
			}
		}
	}
	//returns [qlTableName[parentTableId, jChildData]]
	α SelectSubTables( const vector<TableQL>& tables, const DB::Table& parentTable, DB::WhereClause where )->SubTables{
		SubTables subTables;
		for( auto& qlTable : tables ){//members
			auto fk = findFK( parentTable, qlTable.DBName() );
			DB::Statement statement;
			if( auto map = fk ? fk->Table->Map : nullopt; map ){ //members.member_id  if not a map, get it in main table.
				statement.Select.TryAdd( fk->Table->SurrogateKeys[0] );//add identity_id of members for result.
				columnSql( qlTable, *fk->PKTable, false, statement );
				statement.From.TryAdd( {fk->PKTable->GetPK(), fk, true} ); //identities join members
			}
			else if( auto map = findMap(parentTable, qlTable.DBName()); map ){ //role_members
				auto parent = map->Parent; //role_id
				auto child = map->Child; //permission_id
				statement.Select.TryAdd( parent );
				columnSql( qlTable, *child->PKTable, false, statement ); //select id, allowed, denied
				statement.From.TryAdd( {parent->PKTable->GetPK(), parent, true} ); //from roles join role_members
				statement.From.TryAdd( {child, child->PKTable->GetPK(), true} ); //join permissions
			}
			else
				continue; //THROW_IF( !fk, "Could not find fk for {}->{}", parentTable.Name, qlTable.DBName() );

			statement.Where = where;
			auto& rows = subTables.emplace( qlTable.JsonName, flat_multimap<uint,jobject>{} ).first->second;
			auto forEachRow = [&]( DB::Row&& row ){
				jobject jSubRow;
				let rowToJson2 = [&row, &parentTable]( const vector<ColumnQL>& columns, jobject& jRow ){
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
							jRow[c.JsonName] = ValueToJson( move(row[i++]), &c );
					}
				};
				rowToJson2( qlTable.Columns, jSubRow );
				for( let& childTable : qlTable.Tables ){
					let childDbName = childTable.DBName();
					let pkTable = GetTable( childDbName );
					if( fk ){
						jobject jChildTable;
						rowToJson2( childTable.Columns, jChildTable );
						jSubRow[childTable.JsonName] = jChildTable;
					}
				}
				rows.emplace( row.GetUInt(0), jSubRow );
			};
			auto sql = statement.Move();
			parentTable.Schema->DS()->Select( move(sql), forEachRow );
		}
		return subTables;
	}

	Ω query( const TableQL& ql, DB::Statement&& statement, DB::IDataSource& ds, UserPK /*executer*/, const SubTables* subTables=nullptr, SRCE )ε->jvalue{
		jvalue y;
		if( ql.IsPlural() )
			y = jarray{};
		auto rows = ds.Select( statement.Move(), sl );
		for( auto&& row : rows ){
			auto jrow = ql.ToJson( row, statement.Select.Columns );
			if( subTables && subTables->size() )
				addSubTables( ql, *subTables, jrow, row.GetUInt(0) );
			if( ql.IsPlural() )
				y.get_array().emplace_back( move(jrow) );
			else
				y.emplace_object() = move( jrow );
		}
		return y;
	}

	α Query( const TableQL& ql, DB::Statement&& statement, UserPK executer, SL sl )ε->jvalue{
		Trace{ sl, _tags, "QL Request: {}", ql.ToString() };
		auto dbTable = GetTable( ql.DBName() );
		let result = query( ql, move(statement), *dbTable->Schema->DS(), executer, nullptr, sl );
		Trace{ sl, _tags, "QL Result: {}", serialize(result).substr(0,1024) };
		return result;
	}
}
namespace Jde{
	α QL::SelectStatement( const TableQL& qlTable, optional<bool> includeDeleted )ε->optional<DB::Statement>{
		let dbView = GetTable( qlTable.DBName() );
		DB::Statement statement;
		columnSql( qlTable, *dbView, false, statement, includeDeleted );
		// if( statement.Empty() ) could be a join with only where clause.
		// 	return {};
		if( statement.From.Empty() )
			statement.From += { dbView->Columns[0] };
		auto dbTable = dbView->IsView() ? nullptr : AsTable(dbView);
		if( optional<DB::Criteria> criteria = dbTable && dbTable->Extends ? dbTable->SurrogateKeys[0]->Criteria : nullopt; criteria ) //identities is_group
			statement.Where.Add( *criteria );//group with no members.

		return statement;
	}

	α QL::Query( const TableQL& qlTable, UserPK executer, SL sl )ε->jvalue{
		jvalue y;
		optional<std::pair<bool,up<IException>>> hookExp;
		[sl]( auto& qlTable, auto executer, auto& y, auto& hookExp )->TAwait<optional<jvalue>>::Task {
			try{
				auto j = co_await QL::Hook::Select( qlTable, executer, sl );
				let hasValue = j.has_value();
				if( hasValue )
					y = move( *j );
				hookExp = std::make_pair( hasValue, nullptr );
			}
			catch( IException& e ){
				hookExp = std::make_pair( true, e.Move() );
			}
		}( qlTable, executer, y, hookExp );
		while( !hookExp )
			std::this_thread::yield();
		if( hookExp->second )
			hookExp->second->Throw();
		if( hookExp->first )
			return y;

		let dbTable = GetTable( qlTable.DBName() );
		dbTable->Authorize( Access::ERights::Read, executer, sl );
		auto statement = SelectStatement( qlTable );
/*		columnSql( qlTable, *dbTable, nullptr, flags, false, nullptr, statement, &jsonMembers );

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
		let subTables = SelectSubTables( qlTable.Tables, *DB::AsTable(dbTable), statement ? statement->Where : DB::WhereClause{} );
		if( statement )
			y = query( qlTable, move(*statement), *dbTable->Schema->DS(), executer, &subTables );
		else{
			jobject jRow;
			addSubTables( qlTable, subTables, jRow, 0 );
			y = jRow;
		}
		return y;
	}
	α QL::addSubTables( const TableQL& parentQL, const SubTables& subTables, jobject& parent, uint parentId )ι->void{
//		auto addSubTables = [&]( jobject& jParent, uint id=0 ){
		for( let& qlTable : parentQL.Tables ){
			let subPlural = qlTable.JsonName.ends_with( "s" );
			if( subPlural )
				parent[qlTable.JsonName] = jarray{};
			let pResultTable = subTables.find( qlTable.JsonName );
			if( pResultTable==subTables.end() )
				continue;
			let& subResults = pResultTable->second;
			if( !parentId && subResults.size() )
				parentId = subResults.begin()->first;
			auto range = subResults.equal_range( parentId );
			for( auto pRow = range.first; pRow!=range.second; ++pRow ){
				if( subPlural )
					parent[qlTable.JsonName].get_array().emplace_back( pRow->second );
				else
					parent[qlTable.JsonName] = pRow->second;
			}
		};
	}
}