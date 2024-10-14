#include "GraphQuery.h"
#include <jde/ql/GraphQL.h>
#include <jde/ql/GraphQLHook.h>
#include <jde/db/Database.h>
#include <jde/db/meta/Column.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/generators/WhereClause.h>

#define let const auto

namespace Jde::QL{
	using std::endl;
	constexpr ELogTags _tags{ ELogTags::QL };
	#define _schema DB::DefaultSchema()
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
	α AddColumn( const ColumnQL& c, const TableQL& qlTable, const DB::View& dbView, vector<string>& columns, const DB::View* pDefView, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, str defaultPrefix, bool excludeId, uint* pIndex, vector<Join>& joins, vector<tuple<string,string>>* pJsonMembers, const DB::Syntax& syntax )ε->void{
		auto columnName = DB::Schema::FromJson( c.JsonName );
		if( auto pk = excludeId ? nullptr : dbView.FindPK(); pk && pk->Name==columnName )
			return;
		auto findColumn = []( const DB::View& t, sv n ){ auto p = t.FindColumn( n ); if( !p ) p = t.FindColumn( string{n}+"_id" ); return p; }; //+_id for enums
		auto pSchemaColumn = findColumn( dbView, columnName );
		const DB::Table* dbTable = dynamic_cast<const DB::Table*>( &dbView );
		if( let pExtendedFrom = pDefView || pSchemaColumn || !dbTable ? nullptr : dbTable->GetExtendedFromTable(); pExtendedFrom ){//extension users extends entities
			if( !Join::Contains(joins, dbView.Name) )
				joins.emplace_back( dbView.Name, dbView.GetPK()->Name, pExtendedFrom->Name, pExtendedFrom->GetPK()->Name, true );
			return AddColumn( c, qlTable, *pExtendedFrom, columns, pDefView, dates, flags, pExtendedFrom->Name, excludeId, pIndex, joins, pJsonMembers, syntax );
		}
		else if( !pDefView && pSchemaColumn && pSchemaColumn->PKTable ){//enumeration pSchemaColumn==provider_id
			pDefView = pSchemaColumn->PKTable->QLView
				? pSchemaColumn->PKTable.get() //um_prvoiders
				: pSchemaColumn->PKTable->QLView.get();
			let fkName = pSchemaColumn->Name;
			auto pOther = findColumn( *pDefView, "name" );
			if( !pOther )
				pOther = findColumn( *pDefView, "target" );
			CHECK( pOther );
			columnName = Ƒ( "{}.{} {}", pDefView->Name, pOther->Name, columnName );
			joins.emplace_back( pDefView->Name, pDefView->GetPK()->Name, defaultPrefix, fkName, pSchemaColumn->IsNullable );
		}
		else if( pDefView && !pSchemaColumn ){
			pSchemaColumn = findColumn( *pDefView, columnName );
			if( !pSchemaColumn ){
				pSchemaColumn = findColumn( *pDefView, DB::Schema::ToSingular(columnName) ); THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefView->Name, columnName );
				let p = _schema.Tables.find( pSchemaColumn->PKTable->Name ); THROW_IF( p==_schema.Tables.end(), "Could not find flags pk table for {}.{}", pDefView->Name, columnName );
				flags.emplace( columns.size(), p->second );//*pIndex
			}
			columnName = Ƒ( "{}.{}", pDefView->Name, pSchemaColumn->Name );
		}
		else if( pSchemaColumn ){
			if( pSchemaColumn->IsEnum() ){
				c.SchemaColumnPtr = pSchemaColumn;
				let p = _schema.Tables.find( pSchemaColumn->PKTable->Name ); THROW_IF( p==_schema.Tables.end(), "Could not find flags pk table for {}.{}", pDefView->Name, columnName );
				flags.emplace( *pIndex, p->second );
			}
			else if( pSchemaColumn->QLAppend.size() && !qlTable.FindColumn(pSchemaColumn->QLAppend) ){
				if( CIString name{ Ƒ("{}.{}", defaultPrefix, DB::Schema::FromJson(pSchemaColumn->QLAppend)) }; std::find(columns.begin(), columns.end(), name)==columns.end() ){
					columns.push_back( name );//add db column, doesn't add qlColumn
					c.SchemaColumnPtr = pSchemaColumn;
					//c.SchemaColumnPtr->Table = &dbView; //should be set
				}
			}

			columnName = Ƒ( "{}.{}", defaultPrefix, pSchemaColumn->Name );
		}
		THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefView ? pDefView->Name : "null", columnName );

		let dateTime = pSchemaColumn->Type==DB::EType::DateTime;
		if( dateTime )
			dates.push_back( *pIndex );
		columns.push_back( dateTime ? syntax.DateTimeSelect(columnName) : columnName );
		if( pJsonMembers )
			pJsonMembers->push_back( make_tuple(qlTable.JsonName, c.JsonName) );//DB::Schema::ToJson(pSchemaColumn->Name))

		++(*pIndex);
	}

	α ColumnSql( const TableQL& qlTable, const DB::View& dbView, const DB::View* pDefView, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, str defaultPrefix, bool excludeId, uint* pIndex, vector<Join>& joins, vector<tuple<string,string>>* pJsonMembers, const DB::Syntax& syntax )->string{
		uint index2 = 0;
		if( !pIndex )
			pIndex = &index2;
		vector<string> columns;
		for( let& c : qlTable.Columns )
			AddColumn( c, qlTable, dbView, columns, pDefView, dates, flags, defaultPrefix, excludeId, pIndex, joins, pJsonMembers, syntax );

		for( let& childTable : qlTable.Tables ){
			const DB::View* pkView = &_schema.FindTable( childTable.DBName() );
			sp<DB::Column> pColumn;
			string childPrefix;
			auto findFK = [&pkView]( let& x ){ return x->PKTable && x->PKTable->Name==pkView->Name; };
			auto setFromTable = [&]( const DB::View& t ){
				if( auto pDBColumn = find_if( t.Columns, findFK ); pDBColumn!=t.Columns.end() ){
					pColumn = *pDBColumn;
					childPrefix = t.Name;
				}
				return pColumn!=nullptr;
			};
			if( !setFromTable(dbView) ){
				const DB::Table* dbTable = dynamic_cast<const DB::Table*>( &dbView );
				if( auto pExtendedFromTable = dbTable ? dbTable->GetExtendedFromTable() : nullptr; (!pExtendedFromTable || !setFromTable(*pExtendedFromTable)) && pDefView )
					setFromTable(*pDefView);
			}
			if( pColumn ){
				const DB::Table* pkTable = dynamic_cast<const DB::Table*>( pkView );
				if( pkTable && pkTable->QLView ){
					pkView = pkTable->QLView.get();
				}
				columns.push_back( ColumnSql(childTable, *pkView, nullptr, dates, flags, pkView->Name, false, pIndex, joins, pJsonMembers, syntax) );
				if( !Join::Contains(joins, pkView->Name) )
					joins.emplace_back( pkView->Name, pkView->GetPK()->Name, childPrefix, pColumn->Name, true );
			}
			else
				Error{ _tags, "Could not extract data {}->{}", dbView.Name, childTable.DBName() };
		}
		return Str::AddCommas( columns );
	}
}
namespace Jde{
	α QL::SelectStatement( const TableQL& table, bool includeIdColumn, const DB::Syntax& syntax, string* whereString )ι->tuple<string,vector<DB::Value>>{
		let& schemaTable = _schema.FindTable( table.DBName() );
		vector<uint> dates; flat_map<uint,sp<const DB::Table>> flags;
		vector<tuple<string,string>> jsonMembers;
		vector<Join> joins;
		std::ostringstream sql;
		auto columnSqlValue = ColumnSql( table, schemaTable, nullptr, dates, flags, schemaTable.Name, false, nullptr, joins, &jsonMembers, syntax );
		if( columnSqlValue.empty() )
			return {};

		sql << "select " << columnSqlValue;
		if( let addId = includeIdColumn && table.Tables.size() && !table.FindColumn("id") && table.Columns.size(); addId ) //putting in a map
			sql << ", id";

		auto pExtendedFrom = schemaTable.GetExtendedFromTable();
		let& tableName = pExtendedFrom ? pExtendedFrom->Name : schemaTable.Name;
		sql << endl << "from\t" << tableName;
		for( let& j : joins )
			sql << endl << j.ToString();

		sql << endl << "from\t" << schemaTable.Name;
		for( let& j : joins )
			sql << endl << j.ToString();
		vector<DB::Value> parameters;
		auto where = Where( table, schemaTable, parameters );
		if( pExtendedFrom && schemaTable.GetPK().Criteria.size() ){ //um_entities is_group?
			if( where.size() )
				where += " and ";
			where += schemaTable.GetPK().Criteria;
		}
		if( where.size() )
			sql << endl << "where " << where;
		if( whereString )
			*whereString = where;
		return make_tuple( sql.str(), parameters );
	}
	α QL::Query( const TableQL& table, jobject& jData, UserPK userPK, sp<DB::IDataSource> ds )ε->void{
		ASSERT(ds);
		optional<up<jobject>> hookData;
		[]( auto& table, auto userPK, auto& jData, auto& hookData )->Task {
			AwaitResult result = co_await QL::Hook::Select( table, userPK );
			hookData = result.UP<jobject>();
			if( *hookData )
				jData[table.JsonName] = *(*hookData);
		}( table, userPK, jData, hookData );
		while( !hookData )
			std::this_thread::yield();
		if( *hookData )
			return;
		let isPlural = table.IsPlural();
		let& schemaTable = _schema.FindTable( table.DBName() );
		vector<tuple<string,string>> jsonMembers;
		vector<uint> dates; flat_map<uint,sp<const DB::Table>> flags;
		vector<Join> joins;
		let columnSqlValue = ColumnSql( table, schemaTable, nullptr, dates, flags, schemaTable.Name, false, nullptr, joins, &jsonMembers, ds->Syntax() );
		std::ostringstream sql; //TODO =  SelectStatement( table );
		if( columnSqlValue.size() )
			sql << "select " << columnSqlValue;

		if( let addId = table.Tables.size() && !table.FindColumn("id") && table.Columns.size(); addId ) //Why?
			sql << ", id";
		auto pExtendedFrom = schemaTable.GetExtendedFromTable();
		let& tableName = pExtendedFrom ? pExtendedFrom->Name : schemaTable.Name;
		if( sql.tellp()!=std::streampos(0) ){
			sql << endl << "from\t" << tableName;
			for( let& j : joins )
				sql << endl << j.ToString();
		}
		vector<DB::Value> parameters;
		auto where = Where( table, schemaTable, parameters );
		if( pExtendedFrom && schemaTable.GetPK().Criteria.size() ){ //um_entities is_group
			if( where.size() )
				where += " and ";
			where += schemaTable.GetPK().Criteria;
		}
		auto pAuthorizer = UM::FindAuthorizer( tableName );
		auto colToJson = [&]( const DB::IRow& row, uint iColumn, jobject& obj, sv objectName, sv memberName, const vector<uint>& dates, const flat_map<uint,sp<flat_map<uint,string>>>& flagValues, const ColumnQL* pMember=nullptr )->bool {
			let value{ row[iColumn] };
			if( let index{(DB::EValue)value.index()}; index!=DB::EValue::Null ){
				auto& m = objectName.empty() ? obj[string{memberName}] : obj[string{objectName}][string{memberName}];
				if( index==DB::EValue::UInt64 || index==DB::EEValue:Int32 ){
					if( let pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() ){
						let isEnum{ pMember && pMember->SchemaColumnPtr && pMember->SchemaColumnPtr->IsEnum() };
						if( !isEnum )
							m = JArray{};
						uint remainingFlags = DB::ToUInt( value );
						for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 ){
							if( (remainingFlags & iFlag)==0 )
								continue;
							if( let pValue = pFlagValues->second->find(iFlag); pValue!=pFlagValues->second->end() ){
								if( isEnum )
									m = pValue->second;
								else
									m.push_back( pValue->second );
							}
							else
								m.push_back( std::to_string(iFlag) );
							remainingFlags -= iFlag;
						}
					}
					else if( pAuthorizer && memberName=="id" && !pAuthorizer->CanRead(userPK, (UserPK)DB::ToUInt(value)) )//TODO move to sql
						return false;//TODO uncomment
/*					else if( pMember && pMember->SchemaColumnPtr && pMember->SchemaColumnPtr->IsEnum )
					{
						if( let pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() )
						{
							if( let pValue = pFlagValues->second->find(ToInt(value)); pValue!=pFlagValues->second->end() )
								m.push_back( pValue->second );
							else
								ASSERT( false );
						}
						else
							ASSERT( false );
					}*/
					else
						DB::ToJson( value, m );
				}
				else if( index==DB::EValue::Int64 && find(dates.begin(), dates.end(), iColumn)!=dates.end() )
					m = Jde::DateTime( value.get_int32() ).ToIsoString();
				else
					DB::ToJson( value, m );
			}
			return true;
		};
		flat_map<string,flat_multimap<uint,jobject>> subTables;
		auto selectSubTables = [&subTables, &colToJson, &parameters, &ds]( const vector<TableQL>& tables, const DB::Table& parentTable, sv where2 ){
			for( auto& qlTable : tables ){
				const DB::Table* subTable = &_schema.FindTable( qlTable.DBName() );
				const DB::View* pDefView;
				if( subTable->IsMap() ){//for RolePermissions, subTable=Permissions, defTable=RolePermissions, role_id, permission_id
					let subIsParent = subTable->ChildColumn()->Name==parentTable.FKName();
					auto p = subIsParent ? subTable->ParentTable() : subTable->ChildTable(); THROW_IF( !p, "Could not find {} table for '{}' in schema", (subIsParent ? "parent" : "child"), subTable->Name );
					pDefView = subTable;
					subTable = &*p;
				}
				else{
					auto spDefTable = _schema.FindDefTable( parentTable, *subTable );
					if( !spDefTable ){
						if( auto pExtendedFromTable = parentTable.GetExtendedFromTable(); pExtendedFromTable )
							spDefTable = _schema.FindDefTable( *pExtendedFromTable, *subTable );
						if( !spDefTable )
							continue;  //'um_permissions<->'apis' //THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", parentTable.Name, qlTable.DBName()) );
					}
					pDefView = spDefTable.get();
				}
				let defTableName = pDefView->Name;
				std::ostringstream subSql{ Ƒ("select {0}.{1} primary_id, {0}.{2} sub_id", defTableName, parentTable.FKName(), subTable->FKName()), std::ios::ate };
				let& subTableName = subTable->Name;
				vector<uint> subDates; flat_map<uint,sp<const DB::Table>> subFlags;
				vector<Join> joins;
				let columns = ColumnSql( qlTable, *subTable, pDefView, subDates, subFlags, subTableName, true, nullptr, joins, nullptr, ds->Syntax() );//rolePermissions
				if( columns.size() )
					subSql << ", " << columns;
				subSql << "\nfrom\t" << parentTable.Name << endl
					<< "\tjoin " << defTableName << " on " << parentTable.Name <<".id=" << defTableName << "." << parentTable.FKName();
				if( columns.size() ){
					subSql << "\tjoin " << subTableName << " on " << subTableName <<".id=" << defTableName << "." << subTable->FKName();
					for( let& j : joins )
						subSql << endl << j.ToString();
				}

				if( where2.size() )
					subSql << endl << "where\t" << where2;
				flat_map<uint,sp<flat_map<uint,string>>> subFlagValues;
				for( let& [index,pTable] : subFlags ){
					DB::SelectCacheAwait<flat_map<uint,string>> a = ds->SelectEnum<uint>( pTable->Name );
					subFlagValues[index+2] = SFuture<flat_map<uint,string>>( move(a) ).get();
				}
				auto& rows = subTables.emplace( qlTable.JsonName, flat_multimap<uint,jobject>{} ).first->second;
				auto forEachRow = [&]( const DB::IRow& row ){
					jobject jSubRow;
					uint index = 0;
					let rowToJson2 = [&row, &subDates, &colToJson, &subFlagValues]( const vector<ColumnQL>& columns, bool checkId, jobject& jRow, uint& index2 ){
						for( let& column : columns ){
							auto i = checkId && column.JsonName=="id" ? 1 : (index2++)+2;
							if( column.SchemaColumnPtr && column.SchemaColumnPtr->QLAppend.size() && column.SchemaColumnPtr->Table ){
								let pk = DB::ToUInt( row[i++] ); ++index2;
								let pColumn = column.SchemaColumnPtr->Table->FindColumn( DB::Schema::FromJson(column.SchemaColumnPtr->QLAppend) );  CHECK( pColumn && pColumn->IsEnum() );
								let pEnum = DB::SelectEnumSync( pColumn->PKTable->Name ); CHECK( pEnum->find(pk)!=pEnum->end() );
								colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues, &column );
								let name = jRow[column.JsonName].is_null() ? string{} : jRow[column.JsonName].get<string>();
								jRow[column.JsonName] = name.empty() ? pEnum->find(pk)->second : Ƒ( "{}\\{}", pEnum->find(pk)->second, name );
							}
							else
								colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues, &column );
						}
					};
					rowToJson2( qlTable.Columns, true, jSubRow, index );
					for( let& childTable : qlTable.Tables ){
						let childDbName = childTable.DBName();
						let& pkTable = _schema.FindTable( childDbName );
						let pColumn = find_if( subTable->Columns, [&pkTable](let& x){ return x->PKTable && x->PKTable->Name==pkTable.Name; } );
						if( pColumn!=subTable->Columns.end() ){
							jobject jChildTable;
							rowToJson2( childTable.Columns, false, jChildTable, index );
							jSubRow[childTable.JsonName] = jChildTable;
						}
					}
					rows.emplace( row.GetUInt(0), jSubRow );
				};
				ds->Select( subSql.str(), forEachRow, parameters );// select um_role_permissions.role_id
			}
		};
		selectSubTables( table.Tables, schemaTable, where );
		let jsonTableName = table.JsonName;
		auto addSubTables = [&]( jobject& jParent, uint id=0 ){
			for( let& qlTable : table.Tables ){
				let subPlural = qlTable.JsonName.ends_with( "s" );
				if( subPlural )
					jParent[qlTable.JsonName] = JArray{};
				let pResultTable = subTables.find( qlTable.JsonName );
				if( pResultTable==subTables.end() )
					continue;
				let& subResults = pResultTable->second;
				if( !id && subResults.size() )
					id = subResults.begin()->first;
				auto range = subResults.equal_range( id );
				for( auto pRow = range.first; pRow!=range.second; ++pRow ){
					if( subPlural )
						jParent[qlTable.JsonName].push_back( pRow->second );
					else
						jParent[qlTable.JsonName] = pRow->second;
				}
			}
		};
		if( sql.tellp()!=std::streampos(0) ){
			if( where.size() )
				sql << endl << "where\t" << where;
			if( isPlural )
				jData[jsonTableName] = JArray{};
			auto primaryRow = [&]( const DB::IRow& row ){
				jobject jRow; bool authorized=true;
				for( uint i=0; i<jsonMembers.size() && authorized; ++i ){
					let [parent, column] = jsonMembers[i];
					if( let pColumn = table.JsonName==parent ? table.FindColumn(column) : nullptr; pColumn && pColumn->SchemaColumnPtr && pColumn->SchemaColumnPtr->QLAppend.size() ){
						let pk = DB::ToUInt( row[i++] );
						let pAppend = pColumn->SchemaColumnPtr->Table->FindColumn( DB::Schema::FromJson(pColumn->SchemaColumnPtr->QLAppend) );  CHECK( pAppend && pAppend->IsEnum() );
						let pEnum = DB::SelectEnumSync( pAppend->PKTable->Name ); CHECK( pEnum->find(pk)!=pEnum->end() );
						colToJson( row, i, jRow, {}, column, dates, {} );
						let name = jRow[column].is_null() ? string{} : jRow[column].get<string>();
						jRow[column] = name.empty() ? pEnum->find(pk)->second : Ƒ( "{}\\{}", pEnum->find(pk)->second, name );

					}
					else
					//let column = get<1>( jsonMembers[i] );
						authorized = colToJson( row, i, jRow, parent==jsonTableName ? sv{} : parent, column, dates, {} );
				}
				if( authorized ){
					if( subTables.size() )
						addSubTables( jRow );
					if( isPlural )
						jData[jsonTableName].push_back( jRow );
					else
						jData[jsonTableName] = jRow;
				}
			};
			ds->Select( sql.str(), primaryRow, parameters );
		}
		else{
			jobject jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}
}