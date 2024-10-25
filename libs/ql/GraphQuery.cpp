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
	α FindTable( str tableName )ε->sp<DB::View>;
	//#define _schema DB::DefaultSchema()
/*	struct Join{
		α ToString()Ι->string{ return Ƒ( "{0}join {1} on {1}.{2}={3}.{4}", Left ? "left " : "", NewTable, NewTableId, ExistingTable, ExistingColumn ); }
		Ω Contains( const vector<Join>& joins, str newTable )ι->bool{ return find_if(joins, [&newTable](let& x){ return x.NewTable==newTable; })!=joins.end(); }
		string NewTable;
		string NewTableId;
		string ExistingTable;
		string ExistingColumn;
		bool Left{};
	};
*/
//#define _db DataSource()
//#define _syntax DB::DefaultSyntax()
	α AddColumn( const ColumnQL& c, const TableQL& qlTable, const DB::View& dbTable, DB::Statement& statement, sp<DB::View> defView, flat_map<uint,sp<const DB::Table>>& flags, bool excludeId, uint* pIndex, vector<tuple<string,string>>* pJsonMembers )ε->void{
		auto pk = dbTable.FindPK();
		let isPK = c.JsonName=="id" && pk;
		auto columnName = isPK ? pk->Name : DB::Names::FromJson( c.JsonName );
		if( excludeId && isPK )
			return;
		auto findColumn = []( const DB::View& t, str n ){
			auto p = t.FindColumn( n ); 
			if( !p ) 
				p = t.FindColumn( string{n}+"_id" ); //+_id for enums
			return p; 
		}; 
		auto pSchemaColumn = findColumn( dbTable, columnName );
		if( pSchemaColumn && pSchemaColumn->Table->Name!=dbTable.Name ){//users extends identities
			if( !statement.From.Contains(dbTable.Name) )
				statement.From += { pSchemaColumn->Table->GetPK(), dbTable.GetPK() };
			return AddColumn( c, qlTable, *pSchemaColumn->Table, statement, defView, flags, excludeId, pIndex, pJsonMembers );
		}
		else if( !isPK && !defView && pSchemaColumn && pSchemaColumn->PKTable ){//enumeration pSchemaColumn==provider_id
			defView = pSchemaColumn->PKTable->QLView
				? pSchemaColumn->PKTable->QLView //um_prvoiders
				: pSchemaColumn->PKTable;
			let fkName = pSchemaColumn->Name;
			auto pOther = findColumn( *defView, "name" );
			if( !pOther )
				pOther = findColumn( *defView, "target" );
			CHECK( pOther );
			statement.From += { defView->GetPK(), pOther };
			//joins.emplace_back( defView->Name, defView->GetPK()->Name, defaultPrefix, fkName, pSchemaColumn->IsNullable );
		}
		else if( defView && !pSchemaColumn ){
			pSchemaColumn = findColumn( *defView, columnName );
			if( !pSchemaColumn ){
				pSchemaColumn = findColumn( *defView, DB::Names::ToSingular(columnName) ); THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", defView->Name, columnName );
				flags.emplace( statement.Select.Columns.size(), pSchemaColumn->PKTable );//*pIndex
			}
			columnName = Ƒ( "{}.{}", defView->Name, pSchemaColumn->Name );
		}
		else if( pSchemaColumn ){
			if( pSchemaColumn->IsEnum() ){
				c.DBColumn = pSchemaColumn;
				flags.emplace( *pIndex, pSchemaColumn->PKTable );
			}
			else if( pSchemaColumn->QLAppend.size() && !qlTable.FindColumn(DB::Names::ToJson(pSchemaColumn->QLAppend)) ){
				statement.Select.Add( dbTable.GetColumnPtr(pSchemaColumn->QLAppend) );
				c.DBColumn = pSchemaColumn;
			}
		}
		THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", defView ? defView->Name : "null", columnName );

		statement.Select.Add( pSchemaColumn );
		if( pJsonMembers )
			pJsonMembers->push_back( make_tuple(qlTable.JsonName, c.JsonName) );//DB::Names::ToJson(pSchemaColumn->Name))

		++(*pIndex);
	}

	α ColumnSql( const TableQL& qlTable, const DB::View& dbTable, sp<DB::View> defView, flat_map<uint,sp<const DB::Table>>& flags, bool excludeId, uint* pIndex, DB::Statement& statement, vector<tuple<string,string>>* pJsonMembers )->void{
		uint index2 = 0;
		if( !pIndex )
			pIndex = &index2;
		for( let& c : qlTable.Columns )
			AddColumn( c, qlTable, dbTable, statement, defView, flags, excludeId, pIndex, pJsonMembers );

		for( let& qlChild : qlTable.Tables ){
			auto pkTable = dbTable.Schema->GetViewPtr( qlChild.DBName() );
			auto pFK = dbTable.FindFK( pkTable->Name );
			if( !pFK ){
				if( auto pExtendedFromTable = AsTable(dbTable).GetExtendedFromTable(); pExtendedFromTable ){
					pFK = pExtendedFromTable->FindFK( pkTable->Name );
					if( pFK && !statement.From.Contains(pExtendedFromTable->Name) )
						statement.From+={ pExtendedFromTable->GetPK(), dbTable.GetPK() };
				}
			}
			if( pFK ){
				if( sp<DB::Table> table = AsTable( pkTable ); table && table->QLView )
					pkTable = table->QLView;
				ColumnSql( qlChild, *pkTable, nullptr, flags, false, pIndex, statement, pJsonMembers );
				if( !statement.From.Contains(pkTable->Name) )
					statement.From+={ pFK, pkTable->GetPK() };
			}
			else
				Error{ _tags, "Could not extract data {}->{}", dbTable.Name, qlChild.DBName() };
		}
	}
}
namespace Jde{
	α QL::SelectStatement( sp<DB::AppSchema> schema, const TableQL& qlTable, bool includeIdColumn, optional<DB::WhereClause> where )ι->optional<DB::Statement>{
		let& dbTable = schema->GetTable( qlTable.DBName() );
		flat_map<uint,sp<const DB::Table>> flags;
		vector<tuple<string,string>> jsonMembers;
		DB::Statement statement;
		ColumnSql( qlTable, dbTable, nullptr, flags, false, nullptr, statement, &jsonMembers );
		if( statement.Empty() )
			return {};

		if( let addId = includeIdColumn && qlTable.Tables.size() && !qlTable.FindColumn("id") && qlTable.Columns.size(); addId ) //putting in a map
			statement.Select.Add( dbTable.GetPK() );

		auto pExtendedFrom = dbTable.GetExtendedFromTable();
		statement.Where = QL::ToWhereClause( qlTable, /*pExtendedFrom ? *pExtendedFrom :*/ dbTable );
		if( pExtendedFrom && dbTable.GetPK()->Criteria.size() ) //um_entities is_group
			statement.Where.Add( dbTable.GetPK()->Criteria );

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
		vector<tuple<string,string>> jsonMembers;
		flat_map<uint,sp<const DB::Table>> flags;
		DB::Statement statement;
		ColumnSql( qlTable, *dbTable, nullptr, flags, false, nullptr, statement, &jsonMembers );

		if( let addId = qlTable.Tables.size() && !qlTable.FindColumn("id") && qlTable.Columns.size(); addId ) //Why?
			statement.Select.Add( dbTable->GetPK() );
		auto pExtendedFrom = AsTable(dbTable) ? AsTable(dbTable)->GetExtendedFromTable() : nullptr;
		if( statement.From.Empty() )
			statement.From.SingleTable = dbTable;
		vector<DB::Value> parameters;
		statement.Where = QL::ToWhereClause( qlTable, *dbTable );
		if( pExtendedFrom && dbTable->GetPK()->Criteria.size() ) //um_entities is_group
			statement.Where.Add( dbTable->GetPK()->Criteria );
		auto colToJson = [&]( const DB::IRow& row, uint iColumn, const flat_map<uint,sp<flat_map<uint,string>>>& flagValues, const ColumnQL* pMember=nullptr )->jvalue {
			using enum DB::EValue;
			let dbValue{ row[iColumn] };
			jvalue json;
			let type{ dbValue.Type() };
			if( type==Null )
				return json;
			if( type==UInt64 || type==Int32 ){
				if( let pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() ){
					let isEnum{ pMember && pMember->DBColumn && pMember->DBColumn->IsEnum() };
					if( !isEnum )
						json = jarray{};
					uint remainingFlags = dbValue.ToUInt();
					for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 ){
						if( (remainingFlags & iFlag)==0 )
							continue;
						if( let pValue = pFlagValues->second->find(iFlag); pValue!=pFlagValues->second->end() ){
							if( isEnum )
								json = pValue->second;
							else
								json.get_array().emplace_back( pValue->second );
						}
						else
							json.get_array().emplace_back( std::to_string(iFlag) );
						remainingFlags -= iFlag;
					}
				}
				else
					json = dbValue.ToJson();
			}
			else if( type==Time )
				json = DateTime( dbValue.get_time() ).ToIsoString();
			else
				json = dbValue.ToJson();
			return json;
		};
		flat_map<string,flat_multimap<uint,jobject>> subTables;
		auto selectSubTables = [&subTables, &colToJson, &parameters]( const vector<TableQL>& tables, const DB::Table& parentTable, DB::WhereClause where ){
			for( auto& qlTable : tables ){
				sp<DB::Table> subDBTable = DB::AsTable( FindTable(qlTable.DBName()) );
				sp<DB::View> defView;
				if( subDBTable->IsMap() ){//for RolePermissions, subDBTable=Permissions, defTable=RolePermissions, role_id, permission_id
					let subIsParent = subDBTable->ChildColumn()->Name==parentTable.FKName();
					auto p = subIsParent ? subDBTable->ParentTable() : subDBTable->ChildTable(); THROW_IF( !p, "Could not find {} table for '{}' in schema", (subIsParent ? "parent" : "child"), subDBTable->Name );
					defView = subDBTable;
					subDBTable = p;
				}
				else{
					auto spDefTable = parentTable.Schema->FindDefTable( parentTable, *subDBTable );
					if( !spDefTable ){
						if( auto pExtendedFromTable = parentTable.GetExtendedFromTable(); pExtendedFromTable )
							spDefTable = parentTable.Schema->FindDefTable( *pExtendedFromTable, *subDBTable );
						if( !spDefTable )
							continue;  //'um_permissions<->'apis' //THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", parentTable.Name, qlTable.DBName()) );
					}
					defView = spDefTable;
				}
				flat_map<uint,sp<const DB::Table>> subFlags;
				DB::Statement statement;
				ColumnSql( qlTable, *subDBTable, defView, subFlags, true, nullptr, statement, nullptr );//rolePermissions
				statement.From += { parentTable.GetPK(), subDBTable->GetColumnPtr(parentTable.GetPK()->Name) };
				flat_map<uint,sp<flat_map<uint,string>>> subFlagValues;
				for( let& [index,pTable] : subFlags ){
					DB::SelectCacheAwait<flat_map<uint,string>> a = subDBTable->Schema->DS()->SelectEnum<uint>( pTable->Name );
					subFlagValues[index+2] = SFuture<flat_map<uint,string>>( move(a) ).get();
				}
				auto& rows = subTables.emplace( qlTable.JsonName, flat_multimap<uint,jobject>{} ).first->second;
				auto forEachRow = [&]( const DB::IRow& row ){
					jobject jSubRow;
					uint index = 0;
					let rowToJson2 = [&row, &colToJson, &subFlagValues, &parentTable]( const vector<ColumnQL>& columns, bool checkId, jobject& jRow, uint& index2 ){
						for( let& c : columns ){
							auto i = checkId && c.DBColumn->IsPK() ? 1 : (index2++)+2;
							if( c.DBColumn->QLAppend.size() ){
								let pk = row[i++].ToUInt(); ++index2;
								let pColumn = c.DBColumn->Table->FindColumn( DB::Names::FromJson(c.DBColumn->QLAppend) );  CHECK( pColumn && pColumn->IsEnum() );
								let pEnum = parentTable.Schema->DS()->SelectEnumSync<uint,string>( pColumn->PKTable->Name ); CHECK( pEnum->find(pk)!=pEnum->end() );
								//jRow[c.JsonName] = colToJson( row, i, subFlagValues, &c );
								let name = Json::FindDefaultSV( jRow, c.JsonName );
								jRow[c.JsonName] = name.empty() ? pEnum->find(pk)->second : Ƒ( "{}\\{}", pEnum->find(pk)->second, name );
							}
							else
								jRow[c.JsonName] = colToJson( row, i, subFlagValues, &c );
						}
					};
					rowToJson2( qlTable.Columns, true, jSubRow, index );
					for( let& childTable : qlTable.Tables ){
						let childDbName = childTable.DBName();
						let pkTable = FindTable( childDbName );
						if( auto fk = subDBTable->FindFK(pkTable->Name); fk ){
							jobject jChildTable;
							rowToJson2( childTable.Columns, false, jChildTable, index );
							jSubRow[childTable.JsonName] = jChildTable;
						}
					}
					rows.emplace( row.GetUInt(0), jSubRow );
				};
				auto sql = statement.Move();
				parentTable.Schema->DS()->Select( move(sql.Text), forEachRow, move(sql.Params) );// select um_role_permissions.role_id
			}
		};
		selectSubTables( qlTable.Tables, *DB::AsTable(dbTable), statement.Where );
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
			auto primaryRow = [&]( const DB::IRow& row ){
				jobject jRow; bool authorized=true;
				for( uint i=0; i<jsonMembers.size() && authorized; ++i ){
					let [parent, column] = jsonMembers[i];
					if( let pColumn = qlTable.JsonName==parent ? qlTable.FindColumn(column) : nullptr; pColumn && pColumn->DBColumn && pColumn->DBColumn->QLAppend.size() ){
						let pk = row[i++].ToUInt();
						let pAppend = pColumn->DBColumn->Table->FindColumn( DB::Names::FromJson(pColumn->DBColumn->QLAppend) );  CHECK( pAppend && pAppend->IsEnum() );
						let pEnum = dbTable->Schema->DS()->SelectEnumSync<uint,string>( pAppend->PKTable->Name ); CHECK( pEnum->find(pk)!=pEnum->end() );
						let name = Json::AsSV( colToJson(row, i, {}) );
						jRow[column] = name.empty() ? pEnum->find(pk)->second : Ƒ( "{}\\{}", pEnum->find(pk)->second, name );

					}
					else
						jRow[column] = colToJson( row, i, {}, pColumn );
				}
				if( authorized ){
					if( subTables.size() )
						addSubTables( jRow );
					if( isPlural )
						jData[jsonTableName].get_array().emplace_back( jRow );
					else
						jData[jsonTableName] = jRow;
				}
			};
			auto sql = statement.Move();
			dbTable->Schema->DS()->Select( move(sql.Text), primaryRow, move(sql.Params) );
		}
		else{
			jobject jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}
}