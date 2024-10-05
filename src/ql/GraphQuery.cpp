#include "GraphQuery.h"
#include <jde/ql/GraphQL.h>
#include <jde/ql/GraphQLHook.h>
#include <jde/db//Database.h>
#include <jde/db/syntax/Syntax.h>

#define var const auto

namespace Jde::QL{
	using std::endl;
	constexpr ELogTags _tags{ ELogTags::QL };
	#define _schema DB::DefaultSchema()
	struct Join{
		α ToString()Ι->string{ return Jde::format( "{0}join {1} on {1}.{2}={3}.{4}", Left ? "left " : "", NewTable, NewTableId, ExistingTable, ExistingColumn ); }
		Ω Contains( const vector<Join>& joins, str newTable )ι->bool{ return find_if(joins, [&newTable](var& x){ return x.NewTable==newTable; })!=joins.end(); }
		string NewTable;
		string NewTableId;
		string ExistingTable;
		string ExistingColumn;
		bool Left{};
	};

	α Where( const TableQL& table, const DB::Table& schemaTable, vector<DB::object>& parameters )ε->string{//TODO use FilterQL
		var pWhere = table.Args.find( "filter" );
		var j = pWhere==table.Args.end() ? table.Args : *pWhere;
		ostringstream where;
		for( var& [name,value] : j.items() ){
			var columnName = DB::Schema::FromJson( name );
			auto pColumn = schemaTable.FindColumn( columnName );
			var* tableName = &schemaTable.Name;
			if( var pExtendedFrom = !pColumn ? schemaTable.GetExtendedFromTable(_schema) : nullptr; pExtendedFrom ){
				pColumn = pExtendedFrom->FindColumn( columnName );
				tableName = &pExtendedFrom->Name;
			}
			THROW_IF( !pColumn, "column '{}.{}' not found.", schemaTable.Name, columnName );
			if( where.tellp()!=std::streampos(0) )
				where << endl << "\tand ";
			where << *tableName << "." << columnName;
			sv op = "=";
			const json* pJson{};
			if( value.is_string() || value.is_number() )
				pJson = &value;
			else if( value.is_null() )
				where << " is null";
			else if( value.is_object() && value.items().begin()!=value.items().end() ){
				var first = value.items().begin();
				if( first.value().is_null() )
					where << " is " << (first.key()=="eq" ? "" : "not ") << "null";
				else{
					if( first.key()=="ne" )
						op = "!=";
					pJson = &first.value();
				}
			}
			else if( value.is_array() ){
				where << " in (";
				for( var& v : value )
					parameters.push_back( ToObject(pColumn->Type, v, name) );
				where << Str::AddCommas( vector<string>( value.size(), "?" ) ) << ")";
			}
			else
				THROW("Invalid filter value type '{}'.", value.type_name() );
			if( pJson ){
				where << op << "?";
				parameters.push_back( ToObject(pColumn->Type, *pJson, name) );
			}
		}
		return where.str();
	}
//#define _db DataSource()
//#define _syntax DB::DefaultSyntax()
	α AddColumn( const ColumnQL& c, const TableQL& qlTable, const DB::Table& dbTable, vector<string>& columns, const DB::Table* pDefTable, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, str defaultPrefix, bool excludeId, uint* pIndex, vector<Join>& joins, vector<tuple<string,string>>* pJsonMembers, const DB::Syntax& syntax )->void{
		auto columnName = DB::Schema::FromJson( c.JsonName );
		if( columnName=="id" && excludeId )
			return;
		auto findColumn = []( const DB::Table& t, sv n ){ auto p = t.FindColumn( n ); if( !p ) p = t.FindColumn( string{n}+"_id" ); return p; }; //+_id for enums
		auto pSchemaColumn = findColumn( dbTable, columnName );
		if( var pExtendedFrom = pDefTable || pSchemaColumn ? nullptr : dbTable.GetExtendedFromTable( _schema ); pExtendedFrom ){//extension users extends entities
			if( !Join::Contains(joins, dbTable.Name) )
				joins.emplace_back( dbTable.Name, dbTable.SurrogateKey().Name, pExtendedFrom->Name, pExtendedFrom->SurrogateKey().Name, true );
			return AddColumn( c, qlTable, *pExtendedFrom, columns, pDefTable, dates, flags, pExtendedFrom->Name, excludeId, pIndex, joins, pJsonMembers, syntax );
		}
		else if( !pDefTable && pSchemaColumn && pSchemaColumn->PKTable.size() ){//enumeration pSchemaColumn==provider_id
			pDefTable = &_schema.FindTable( pSchemaColumn->PKTable ); //um_prvoiders
			if( pDefTable->QLView.size() )
				pDefTable = &_schema.FindTable( pDefTable->QLView );
			var fkName = pSchemaColumn->Name;
			auto pOther = findColumn( *pDefTable, "name" );
			if( !pOther )
				pOther = findColumn( *pDefTable, "target" );
			CHECK( pOther );
			columnName = Jde::format( "{}.{} {}", pDefTable->Name, pOther->Name, columnName );
			joins.emplace_back( pDefTable->Name, pDefTable->SurrogateKey().Name, defaultPrefix, fkName, pSchemaColumn->IsNullable );
		}
		else if( pDefTable && !pSchemaColumn ){
			pSchemaColumn = findColumn( *pDefTable, columnName );
			if( !pSchemaColumn ){
				pSchemaColumn = findColumn( *pDefTable, DB::Schema::ToSingular(columnName) ); THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefTable->Name, columnName );
				var p = _schema.Tables.find( pSchemaColumn->PKTable ); THROW_IF( p==_schema.Tables.end(), "Could not find flags pk table for {}.{}", pDefTable->Name, columnName );
				flags.emplace( columns.size(), p->second );//*pIndex
			}
			columnName = Jde::format( "{}.{}", pDefTable->Name, pSchemaColumn->Name );
		}
		else if( pSchemaColumn ){
			if( pSchemaColumn->IsEnum ){
				c.SchemaColumnPtr = pSchemaColumn;
				var p = _schema.Tables.find( pSchemaColumn->PKTable ); THROW_IF( p==_schema.Tables.end(), "Could not find flags pk table for {}.{}", pDefTable->Name, columnName );
				flags.emplace( *pIndex, p->second );
			}
			else if( pSchemaColumn->QLAppend.size() && !qlTable.FindColumn(pSchemaColumn->QLAppend) ){
				if( CIString name{ Jde::format("{}.{}", defaultPrefix, DB::Schema::FromJson(pSchemaColumn->QLAppend)) }; std::find(columns.begin(), columns.end(), name)==columns.end() )
				{
					columns.push_back( name );//add db column, doesn't add qlColumn
					c.SchemaColumnPtr = pSchemaColumn;
					c.SchemaColumnPtr->TablePtr = &dbTable;
				}
			}

			columnName = Jde::format( "{}.{}", defaultPrefix, pSchemaColumn->Name );
		}
		THROW_IF( !pSchemaColumn, "Could not find column '{}.{}'", pDefTable ? pDefTable->Name : "null", columnName );

		var dateTime = pSchemaColumn->Type==DB::EType::DateTime;
		if( dateTime )
			dates.push_back( *pIndex );
		columns.push_back( dateTime ? syntax.DateTimeSelect(columnName) : columnName );
		if( pJsonMembers )
			pJsonMembers->push_back( make_tuple(qlTable.JsonName, c.JsonName) );//DB::Schema::ToJson(pSchemaColumn->Name))

		++(*pIndex);
	}

	α ColumnSql( const TableQL& qlTable, const DB::Table& dbTable, const DB::Table* pDefTable, vector<uint>& dates, flat_map<uint,sp<const DB::Table>>& flags, str defaultPrefix, bool excludeId, uint* pIndex, vector<Join>& joins, vector<tuple<string,string>>* pJsonMembers, const DB::Syntax& syntax )->string{
		uint index2 = 0;
		if( !pIndex )
			pIndex = &index2;
		vector<string> columns;
		for( var& c : qlTable.Columns )
			AddColumn( c, qlTable, dbTable, columns, pDefTable, dates, flags, defaultPrefix, excludeId, pIndex, joins, pJsonMembers, syntax );

		for( var& childTable : qlTable.Tables ){
			auto* pkTable = &_schema.FindTableSuffix( childTable.DBName() );
			const DB::Column* pColumn = nullptr;
			string childPrefix;
			auto findFK = [&pkTable]( var& x ){ return x.PKTable==pkTable->Name; };
			auto setFromTable = [&]( const DB::Table& t ){
				if( auto pDBColumn = find_if( t.Columns, findFK ); pDBColumn!=t.Columns.end() ){
					pColumn = &*pDBColumn;
					childPrefix = t.Name;
				}
				return pColumn!=nullptr;
			};
			if( !setFromTable(dbTable) ){
				if( auto pExtendedFromTable = dbTable.GetExtendedFromTable(_schema); (!pExtendedFromTable || !setFromTable(*pExtendedFromTable)) && pDefTable )
					setFromTable(*pDefTable);
			}
			if( pColumn ){
				if( auto pView = pkTable->QLView.size() ? _schema.Tables.find(pkTable->QLView) : _schema.Tables.end(); pView!=_schema.Tables.end() )
					pkTable = &*pView->second;
				columns.push_back( ColumnSql(childTable, *pkTable, nullptr, dates, flags, pkTable->Name, false, pIndex, joins, pJsonMembers, syntax) );
				if( !Join::Contains(joins, pkTable->Name) )
					joins.emplace_back( pkTable->Name, pkTable->SurrogateKey().Name, childPrefix, pColumn->Name, true );
			}
			else
				Error( _tags, "Could not extract data {}->{}", dbTable.Name, childTable.DBName() );
		}
		return Str::AddCommas( columns );
	}
}
namespace Jde{
	α QL::SelectStatement( const TableQL& table, bool includeIdColumn, const DB::Syntax& syntax, string* whereString )ι->tuple<string,vector<DB::object>>{
		var& schemaTable = _schema.FindTableSuffix( table.DBName() );
		vector<uint> dates; flat_map<uint,sp<const DB::Table>> flags;
		vector<tuple<string,string>> jsonMembers;
		vector<Join> joins;
		ostringstream sql;
		auto columnSqlValue = ColumnSql( table, schemaTable, nullptr, dates, flags, schemaTable.Name, false, nullptr, joins, &jsonMembers, syntax );
		if( columnSqlValue.empty() )
			return {};

		sql << "select " << columnSqlValue;
		if( var addId = includeIdColumn && table.Tables.size() && !table.FindColumn("id") && table.Columns.size(); addId ) //putting in a map
			sql << ", id";

		auto pExtendedFrom = schemaTable.GetExtendedFromTable( _schema );
		var& tableName = pExtendedFrom ? pExtendedFrom->Name : schemaTable.Name;
		sql << endl << "from\t" << tableName;
		for( var& j : joins )
			sql << endl << j.ToString();

		sql << endl << "from\t" << schemaTable.Name;
		for( var& j : joins )
			sql << endl << j.ToString();
		vector<DB::object> parameters;
		auto where = Where( table, schemaTable, parameters );
		if( pExtendedFrom && schemaTable.SurrogateKey().Criteria.size() ){ //um_entities is_group?
			if( where.size() )
				where += " and ";
			where += schemaTable.SurrogateKey().Criteria;
		}
		if( where.size() )
			sql << endl << "where " << where;
		if( whereString )
			*whereString = where;
		return make_tuple( sql.str(), parameters );
	}
	α QL::Query( const TableQL& table, json& jData, UserPK userPK, sp<DB::IDataSource> ds )ε->void{
		ASSERT(ds);
		optional<up<json>> hookData;
		[]( auto& table, auto userPK, auto& jData, auto& hookData )->Task {
			AwaitResult result = co_await QL::Hook::Select( table, userPK );
			hookData = result.UP<json>();
			if( *hookData )
				jData[table.JsonName] = *(*hookData);
		}( table, userPK, jData, hookData );
		while( !hookData )
			std::this_thread::yield();
		if( *hookData )
			return;
		var isPlural = table.IsPlural();
		var& schemaTable = _schema.FindTableSuffix( table.DBName() );
		vector<tuple<string,string>> jsonMembers;
		vector<uint> dates; flat_map<uint,sp<const DB::Table>> flags;
		vector<Join> joins;
		var columnSqlValue = ColumnSql( table, schemaTable, nullptr, dates, flags, schemaTable.Name, false, nullptr, joins, &jsonMembers, ds->Syntax() );
		ostringstream sql; //TODO =  SelectStatement( table );
		if( columnSqlValue.size() )
			sql << "select " << columnSqlValue;

		if( var addId = table.Tables.size() && !table.FindColumn("id") && table.Columns.size(); addId ) //Why?
			sql << ", id";
		auto pExtendedFrom = schemaTable.GetExtendedFromTable( _schema );
		var& tableName = pExtendedFrom ? pExtendedFrom->Name : schemaTable.Name;
		if( sql.tellp()!=std::streampos(0) ){
			sql << endl << "from\t" << tableName;
			for( var& j : joins )
				sql << endl << j.ToString();
		}
		vector<DB::object> parameters;
		auto where = Where( table, schemaTable, parameters );
		if( pExtendedFrom && schemaTable.SurrogateKey().Criteria.size() ){ //um_entities is_group?
			if( where.size() )
				where += " and ";
			where += schemaTable.SurrogateKey().Criteria;
		}
		auto pAuthorizer = UM::FindAuthorizer( tableName );
		auto colToJson = [&]( const DB::IRow& row, uint iColumn, json& obj, sv objectName, sv memberName, const vector<uint>& dates, const flat_map<uint,sp<flat_map<uint,string>>>& flagValues, const ColumnQL* pMember=nullptr )->bool {
			var value{ row[iColumn] };
			if( var index{(DB::EObject)value.index()}; index!=DB::EObject::Null ){
				auto& m = objectName.empty() ? obj[string{memberName}] : obj[string{objectName}][string{memberName}];
				if( index==DB::EObject::UInt64 || index==DB::EObject::Int32 ){
					if( var pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() ){
						var isEnum{ pMember && pMember->SchemaColumnPtr && pMember->SchemaColumnPtr->IsEnum };
						if( !isEnum )
							m = json::array();
						uint remainingFlags = DB::ToUInt( value );
						for( uint iFlag=0x1; remainingFlags!=0; iFlag <<= 1 ){
							if( (remainingFlags & iFlag)==0 )
								continue;
							if( var pValue = pFlagValues->second->find(iFlag); pValue!=pFlagValues->second->end() ){
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
						if( var pFlagValues = flagValues.find(iColumn); pFlagValues!=flagValues.end() )
						{
							if( var pValue = pFlagValues->second->find(ToInt(value)); pValue!=pFlagValues->second->end() )
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
				else if( index==DB::EObject::Int64 && find(dates.begin(), dates.end(), iColumn)!=dates.end() )
					m = Jde::DateTime( get<_int>(value) ).ToIsoString();
				else
					DB::ToJson( value, m );
			}
			return true;
		};
		flat_map<string,flat_multimap<uint,json>> subTables;
		auto selectSubTables = [&subTables, &colToJson, &parameters, &ds]( const vector<TableQL>& tables, const DB::Table& parentTable, sv where2 ){
			for( auto& qlTable : tables ){
				auto pSubTable = &_schema.FindTableSuffix( qlTable.DBName() );
				const DB::Table* pDefTable;
				if( pSubTable->IsMap(_schema) ){//for RolePermissions, subTable=Permissions, defTable=RolePermissions, role_id, permission_id
					pDefTable = pSubTable;
					var subIsParent = pDefTable->ChildColumn()->Name==parentTable.FKName();
					auto p = subIsParent ? pDefTable->ParentTable( _schema ) : pDefTable->ChildTable( _schema ); THROW_IF( !p, "Could not find {} table for '{}' in schema", (subIsParent ? "parent" : "child"), pDefTable->Name );
					pSubTable = &*p;
				}
				else{
					auto spDefTable = _schema.FindDefTable( parentTable, *pSubTable );
					if( !spDefTable ){
						if( auto pExtendedFromTable = parentTable.GetExtendedFromTable( _schema ); pExtendedFromTable )
							spDefTable = _schema.FindDefTable( *pExtendedFromTable, *pSubTable );
						if( !spDefTable )
							continue;  //'um_permissions<->'apis' //THROW_IF( !pDefTable, Exception("Could not find def table '{}<->'{}' in schema", parentTable.Name, qlTable.DBName()) );
					}
					pDefTable = spDefTable.get();
				}
				var subTable = *pSubTable;
				var defTableName = pDefTable->Name;
				ostringstream subSql{ Jde::format("select {0}.{1} primary_id, {0}.{2} sub_id", defTableName, parentTable.FKName(), subTable.FKName()), std::ios::ate };
				var& subTableName = subTable.Name;
				vector<uint> subDates; flat_map<uint,sp<const DB::Table>> subFlags;
				vector<Join> joins;
				var columns = ColumnSql( qlTable, subTable, pDefTable, subDates, subFlags, subTableName, true, nullptr, joins, nullptr, ds->Syntax() );//rolePermissions
				if( columns.size() )
					subSql << ", " << columns;
				subSql << "\nfrom\t" << parentTable.Name << endl
					<< "\tjoin " << defTableName << " on " << parentTable.Name <<".id=" << defTableName << "." << parentTable.FKName();
				if( columns.size() ){
					subSql << "\tjoin " << subTableName << " on " << subTableName <<".id=" << defTableName << "." << subTable.FKName();
					for( var& j : joins )
						subSql << endl << j.ToString();
				}

				if( where2.size() )
					subSql << endl << "where\t" << where2;
				flat_map<uint,sp<flat_map<uint,string>>> subFlagValues;
				for( var& [index,pTable] : subFlags ){
					DB::SelectCacheAwait<flat_map<uint,string>> a = ds->SelectEnum<uint>( pTable->Name );
					subFlagValues[index+2] = SFuture<flat_map<uint,string>>( move(a) ).get();
				}
				auto& rows = subTables.emplace( qlTable.JsonName, flat_multimap<uint,json>{} ).first->second;
				auto forEachRow = [&]( const DB::IRow& row ){
					json jSubRow;
					uint index = 0;
					var rowToJson2 = [&row, &subDates, &colToJson, &subFlagValues]( const vector<ColumnQL>& columns, bool checkId, json& jRow, uint& index2 ){
						for( var& column : columns ){
							auto i = checkId && column.JsonName=="id" ? 1 : (index2++)+2;
							if( column.SchemaColumnPtr && column.SchemaColumnPtr->QLAppend.size() && column.SchemaColumnPtr->TablePtr ){
								var pk = DB::ToUInt( row[i++] ); ++index2;
								var pColumn = column.SchemaColumnPtr->TablePtr->FindColumn( DB::Schema::FromJson(column.SchemaColumnPtr->QLAppend) );  CHECK( pColumn && pColumn->IsEnum );
								var pEnum = DB::SelectEnumSync( pColumn->PKTable ); CHECK( pEnum->find(pk)!=pEnum->end() );
								colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues, &column );
								var name = jRow[column.JsonName].is_null() ? string{} : jRow[column.JsonName].get<string>();
								jRow[column.JsonName] = name.empty() ? pEnum->find(pk)->second : Jde::format( "{}\\{}", pEnum->find(pk)->second, name );
							}
							else
								colToJson( row, i, jRow, "", column.JsonName, subDates, subFlagValues, &column );
						}
					};
					rowToJson2( qlTable.Columns, true, jSubRow, index );
					for( var& childTable : qlTable.Tables ){
						var childDbName = childTable.DBName();
						var& pkTable = _schema.FindTableSuffix( childDbName );
						var pColumn = find_if( subTable.Columns.begin(), subTable.Columns.end(), [&pkTable](var& x){ return x.PKTable==pkTable.Name; } );
						if( pColumn!=subTable.Columns.end() ){
							json jChildTable;
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
		var jsonTableName = table.JsonName;
		auto addSubTables = [&]( json& jParent, uint id=0 ){
			for( var& qlTable : table.Tables ){
				var subPlural = qlTable.JsonName.ends_with( "s" );
				if( subPlural )
					jParent[qlTable.JsonName] = json::array();
				var pResultTable = subTables.find( qlTable.JsonName );
				if( pResultTable==subTables.end() )
					continue;
				var& subResults = pResultTable->second;
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
				jData[jsonTableName] = json::array();
			auto primaryRow = [&]( const DB::IRow& row ){
				json jRow; bool authorized=true;
				for( uint i=0; i<jsonMembers.size() && authorized; ++i ){
					var [parent, column] = jsonMembers[i];
					if( var pColumn = table.JsonName==parent ? table.FindColumn(column) : nullptr; pColumn && pColumn->SchemaColumnPtr && pColumn->SchemaColumnPtr->QLAppend.size() ){
						var pk = DB::ToUInt( row[i++] );
						var pAppend = pColumn->SchemaColumnPtr->TablePtr->FindColumn( DB::Schema::FromJson(pColumn->SchemaColumnPtr->QLAppend) );  CHECK( pAppend && pAppend->IsEnum );
						var pEnum = DB::SelectEnumSync( pAppend->PKTable ); CHECK( pEnum->find(pk)!=pEnum->end() );
						colToJson( row, i, jRow, {}, column, dates, {} );
						var name = jRow[column].is_null() ? string{} : jRow[column].get<string>();
						jRow[column] = name.empty() ? pEnum->find(pk)->second : Jde::format( "{}\\{}", pEnum->find(pk)->second, name );

					}
					else
					//var column = get<1>( jsonMembers[i] );
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
			json jRow;
			addSubTables( jRow );
			jData[jsonTableName] = jRow;
		}
	}
}