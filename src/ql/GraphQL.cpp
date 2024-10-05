#include <jde/ql/GraphQL.h>
#include <jde/Str.h>
#include <jde/io/Json.h>
#include <jde/ql/GraphQLHook.h>
#include <jde/coroutine/TaskOld.h>
#include <jde/ql/Introspection.h>
#include <jde/db/metadata/SchemaProc.h>
#include <jde/db/Row.h>
#include <jde/db/Database.h>
#include <jde/db/DataSource.h>
#include <jde/db/syntax/Syntax.h>
#include "../../../Framework/source/DateTime.h"
#include "GraphQuery.h"
#include <jde/db/syntax/WhereClause.h>
#include "ops/Insert.h"
#include "ops/Purge.h"
#include "types/Parser.h"

#define var const auto
namespace Jde{
	using nlohmann::json;
	using DB::EObject;
	using DB::EType;
	using QL::QLFieldKind;
//#define	_pDataSource DB::DataSourcePtr()
//#define  _schema DB::DefaultSchema()
	constexpr ELogTags _tags{ ELogTags::QL };

/*
	α DB::SetQLDataSource( sp<DB::IDataSource> p )ι->void{ _pDataSource = p; }

	α DB::ClearQLDataSource()ι->void{ _pDataSource = nullptr; }
	//MutationListeners - deprecated, use hooks.

	up<flat_map<string,vector<function<void(const MutationQL& m, PK id)>>>> _applyMutationListeners;  shared_mutex _applyMutationListenerMutex;
	α ApplyMutationListeners()ι->flat_map<string,vector<function<void(const MutationQL& m, PK id)>>>&{ if( !_applyMutationListeners ) _applyMutationListeners = mu<flat_map<string,vector<function<void(const MutationQL& m, PK id)>>>>(); return *_applyMutationListeners; }

	α DB::AddMutationListener( string tablePrefix, function<void(const MutationQL& m, PK id)> listener )ι->void{
		unique_lock l{_applyMutationListenerMutex};
		auto& listeners = ApplyMutationListeners().try_emplace( move(tablePrefix) ).first->second;
		listeners.push_back( listener );
	}

	namespace DB{
		α GraphQL::DataSource()ι->sp<IDataSource>{ return _pDataSource; }
	}
	α DB::AppendQLDBSchema( const DB::Schema& schema )ι->void{
		for( var& [name,v] : schema.Types )
			_schema.Types.emplace( name, v );
		for( var& [name,v] : schema.Tables )
			_schema.Tables.emplace( name, v );
	}

	α DB::SetQLIntrospection( json&& j )ε->void{
		_introspection = mu<GraphQL::Introspection>( move(j) );
	}
*/
//namespace DB{
	α QL::ToJsonName( DB::Column c, const DB::Schema& schema )ε->tuple<string,string>{
		string tableName;
		auto memberName{ DB::Schema::ToJson(c.Name) };
		if( c.IsFlags || c.IsEnum ){
			var pkTable = schema.FindTable( c.PKTable );
			tableName = pkTable.Name;
			memberName = DB::Schema::ToJson( pkTable.NameWithoutType() );
			if( c.IsEnum )
				memberName = DB::Schema::ToSingular( memberName );
		}
		return make_tuple( memberName, tableName );
	}
	α QL::ParseQL( sv query )ε->RequestQL{
		return Parse( query );
	}
namespace QL{
	α SubWhere( const DB::Table& table, const DB::Column& c, vector<DB::object>& params, uint paramIndex )ε->string{
		ostringstream sql{ "=( select id from ", std::ios::ate }; sql << table.Name << " where " << c.Name;
		if( c.QLAppend.size() ){
			CHECK( paramIndex<params.size() && params[paramIndex].index()==(uint)EObject::String );
			var split = Str::Split( get<string>(params.back()), "\\" ); CHECK( split.size() );
			var appendColumnName = DB::Schema::FromJson( c.QLAppend );
			var pColumn = table.FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
			sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
			if( split.size()>1 ){
				params.push_back( split[1] );
				params[paramIndex] = split[0];
			}
		}
		else
			sql << "=? )";
		return sql.str();
	}

	α Update( const DB::Table& table, const MutationQL& m, sp<DB::IDataSource> ds )->tuple<uint,DB::object>{
		var pExtendedFromTable = table.GetExtendedFromTable( ds->Schema() );
		auto [count,rowId] = pExtendedFromTable ? Update(*pExtendedFromTable, m, ds) : make_tuple( 0, 0 );
		ostringstream sql{ Ƒ("update {} set ", table.Name), std::ios::ate };
		vector<DB::object> parameters; parameters.reserve( table.Columns.size() );
		var input = m.Input();
		string sqlUpdate;
		DB::WhereClause where;
		for( var& c : table.Columns ){
			if( !c.Updateable ){
				if( var p=find(table.SurrogateKeys, c.Name); p!=table.SurrogateKeys.end() ){
					if( pExtendedFromTable )
						where.Add( c.Name, rowId );
					else if( var pId = m.Args.find( DB::Schema::ToJson(c.Name) ); pId!=m.Args.end() )
						where.Add( c.Name, rowId=ToObject(EType::ULong, *pId, c.Name) );
					else{
						auto parentColumnName = table.ParentColumn() ? table.ParentColumn()->Name : string{};
						auto pTable = c.Name==parentColumnName ? table.ParentTable( ds->Schema() ) : table.ChildTable( ds->Schema() ); CHECK( pTable );
						auto pValue = m.Args.find( "target" );
						sv cName{ pValue==m.Args.end() ? "name" : "target" };
						if( pValue==m.Args.end() ){
							pValue = m.Args.find( cName ); CHECK( pValue!=m.Args.end() );
						}
						var pNameColumn = pTable->FindColumn( cName ); CHECK( pNameColumn );
						auto whereParams = where.Parameters();
						whereParams.push_back( pValue->get<string>() );
						where << c.Name+SubWhere( *pTable, *pNameColumn, whereParams, whereParams.size()-1 );
					}
					//else
					//	THROW( "Could not get criterial from {}", m.Args.dump() );
				}
				else if( c.Name=="updated" )
					sqlUpdate = Ƒ( ",{}={}", c.Name, ToSV(ds->Syntax().UtcNow()) );
			}
			else{
				var [memberName, tableName] = ToJsonName( c, ds->Schema() );
				var pValue = input.find( memberName );
				if( pValue==input.end() )
					continue;
				if( parameters.size() )
					sql << ", ";
				sql << c.Name << "=?";
				if( c.IsFlags ){
					uint value = 0;
					if( pValue->is_array() && pValue->size() ){
						optional<flat_map<string,uint>> values;
						[] (auto& values, auto& tableName, sp<DB::IDataSource> ds)ι->Coroutine::Task {
							AwaitResult result = co_await ds->SelectMap<string,uint>(Ƒ("select name, id from {}", tableName));
							values = *( result.UP<flat_map<string,uint>>() );
						}(values, tableName, ds);
						while( !values )
							std::this_thread::yield();

						for( var& flag : *pValue ){
							if( var pFlag = values->find(flag); pFlag != values->end() )
								value |= pFlag->second;
						}
					}
					parameters.push_back( value );
				}
				else
					parameters.push_back( ToObject(c.Type, *pValue, memberName) );
			}
		}
		THROW_IF( where.Size()==0, "There is no where clause." );
		THROW_IF( parameters.size()==0 && count==0, "There is nothing to update." );
		uint result{};
		if( parameters.size() ){
			for( var& param : where.Parameters() ) parameters.push_back( param );
			if( sqlUpdate.size() )
				sql << sqlUpdate;
			sql << ' ' << where.Move();

			result = ds->Execute( sql.str(), parameters );
		}
		return make_tuple( count+result, rowId );
	}
	α SetDeleted( const DB::Table& table, uint id, iv value, sp<DB::IDataSource> ds )ε->uint{
		var column = table.FindColumn( "deleted", ds->Schema() );
		vector<DB::object> parameters{ id };
		const string sql{ Ƒ("update {} set deleted={} where id=?", column.TablePtr->Name, value) };
		return ds->Execute( sql, parameters );
	}
	α Delete( const DB::Table& table, const MutationQL& m, sp<DB::IDataSource> ds )ε->uint{
		return SetDeleted( table, m.Id(), ds->Syntax().UtcNow(), ds );
	}
	α Restore( const DB::Table& table, const MutationQL& m, sp<DB::IDataSource> ds )ε->uint{
		return SetDeleted( table, m.Id(), "null", ds );
	}

	α ChildParentParams( sv childId, sv parentId, const json& input )->vector<DB::object>{
		vector<DB::object> parameters;
		if( var p = input.find(childId); p!=input.end() )
			parameters.push_back( ToObject(EType::ULong, *p, childId) );
		else if( var p = input.find("target"); p!=input.end() )
			parameters.push_back( p->get<string>() );
		else
			THROW( "Could not find '{}' or target in '{}'", childId, input.dump() );

		if( var p = input.find(parentId); p!=input.end() )
			parameters.push_back( ToObject(EType::ULong, *p, parentId) );
		else if( var p = input.find("name"); p!=input.end() )
			parameters.push_back( p->get<string>() );
		else
			THROW( "Could not find '{}' or name in '{}'", parentId, input.dump() );

		return parameters;
	};
	α Add( const DB::Table& table, const json& input, sp<DB::IDataSource> ds )->uint{
		string childColName = table.ChildColumn()->Name;
		string parentColName = table.ParentColumn()->Name;
		ostringstream sql{ "insert into ", std::ios::ate }; sql << table.Name << "(" << childColName << "," << parentColName;
		var childId = DB::Schema::ToJson( childColName );
		var parentId = DB::Schema::ToJson( parentColName );
		auto parameters = ChildParentParams( childId, parentId, input );
		ostringstream values{ "?,?", std::ios::ate };
		for( var& [name,value] : input.items() ){
			if( name==childId || name==parentId )
				continue;
			var columnName = DB::Schema::FromJson( name );
			auto pColumn = table.FindColumn( columnName ); THROW_IF(!pColumn, "Could not find column {}.{}.", table.Name, columnName );
			sql << "," << columnName;
			values << ",?";

			parameters.push_back( ToObject(pColumn->Type, value, name) );
		}
		sql << ")values( " << values.str() << ")";
		return ds->Execute( sql.str(), parameters );
	}
	α Remove( const DB::Table& table, const json& input, sp<DB::IDataSource> ds )->uint{
		var& childId = table.ChildColumn()->Name;
		var& parentId = table.ParentColumn()->Name;
		auto params = ChildParentParams( DB::Schema::ToJson(childId), DB::Schema::ToJson(parentId), input );
		ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << childId;
		if( (EObject)params[0].index()==EObject::String ){
			var pTable = table.ChildTable(ds->Schema()); CHECK( pTable );
			var pTarget = pTable->FindColumn( "target" ); CHECK( pTarget );
			sql << SubWhere( *pTable, *pTarget, params, 0 );
		}
		else
			sql << "=?";
		sql << " and " << parentId;
		if( (EObject)params[1].index()==EObject::String ){
			var pTable = table.ParentTable(ds->Schema()); CHECK( pTable );
			var pName = pTable->FindColumn( "name" ); CHECK( pName );
			sql << SubWhere( *pTable, *pName, params, 1 );
/*			sql << "=( select id from " << pTable->Name << " where name";
			if( pName->QLAppend.size() )
			{

				var split = Str::Split( get<string>(params[1]), "\\" ); CHECK( split.size() );
				var appendColumnName = DB::Schema::FromJson( pName->QLAppend );
				var pColumn = pTable->FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
				sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
				if( split.size()>1 )
				{
					params.push_back( split[1] );
					params[1] = split[0];
				}
			}
			else
				sql << "=? )";*/
		}
		else
			sql << "=?";

		return ds->Execute( sql.str(), params );
	}

	α Start( sp<MutationQL> m, UserPK userPK )ε->uint{
		optional<uint> result;
		[&]()ι->MutationAwaits::Task {
			auto await_ = Hook::Start( m, userPK );
			result = co_await await_;
		}();
		while( !result )
			std::this_thread::yield();//TODO remove this when async.
		return *result;
	}

	α Stop( sp<MutationQL> m, UserPK userPK )ε->uint{
		optional<uint> result;
		[&]()ι->MutationAwaits::Task {
			result = co_await Hook::Stop( m, userPK );
		}();
	while( !result )
			std::this_thread::yield();//TODO remove this when async.
		return *result;
	}
#pragma warning( disable : 4701 )
	GraphQL::GraphQL( sp<DB::IDataSource> ds, json* introspection )ι:
		_ds{ ds },
//		_schema{ schema },
//		_syntax{ DB::DefaultSyntax() },
		_introspection{ introspection ? mu<Introspection>( move(*introspection) ) : nullptr }
	{}

	α GraphQL::Mutation( const MutationQL& m, UserPK userPK )ε->uint{
		var& table = _ds->Schema().FindTableSuffix( m.TableSuffix() );
		if( var pAuthorizer = UM::FindAuthorizer(table.Name); pAuthorizer )
			pAuthorizer->Test( m.Type, userPK );
		optional<uint> result;
		switch( m.Type ){
		//using namespace DB;
		using enum EMutationQL;
		case Create:{
			optional<uint> extendedFromId;
			if( var pExtendedFrom = table.GetExtendedFromTable(_ds->Schema()); pExtendedFrom ){
				[](auto extendedFromId, var& pExtendedFrom, var& m, auto userPK, auto ds)ι->Task {
					AwaitResult result = co_await InsertAwait( *pExtendedFrom, m, userPK, 0, ds );
					extendedFromId = *( result.UP<uint>() );
				}(extendedFromId, pExtendedFrom, m, userPK, _ds);
				while (!extendedFromId)
					std::this_thread::yield();
			}
			auto _tag = _tags | ELogTags::Pedantic;
			Trace{ _tag, "calling InsertAwait" };
			[] (auto& table, auto& m, auto userPK, auto extendedFromId, auto& result, auto ds)ι->Task {
				//result = *( co_await InsertAwait(table, m, userPK, extendedFromId.value_or(0)) ).UP<uint>();
				AwaitResult awaitResult = co_await InsertAwait( table, m, userPK, extendedFromId.value_or(0), ds );
				result = *( awaitResult.UP<uint>() );
			}(table, m, userPK, extendedFromId, result, _ds);
			while (!result)
				std::this_thread::yield();
			Trace{ _tag, "~calling InsertAwait" };
		break;}
		case Update:
			result = get<0>( QL::Update(table, m, _ds) );
			break;
		case Delete:
			result = QL::Delete( table, m, _ds );
			break;
		case Restore:
			result = QL::Restore( table, m, _ds );
			break;
		case Purge:
			[] (auto& table, auto& m, auto userPK, auto& result, auto ds )ι->Task {
				//result = *( co_await PurgeAwait{table, m, userPK} ).UP<uint>();
				AwaitResult awaitResult = co_await PurgeAwait{ table, m, userPK, ds };
				result = *( awaitResult.UP<uint>() );
			}( table, m, userPK, result, _ds );
			while (!result)
				std::this_thread::yield();
			break;
		case Add:
			result = QL::Add( table, m.Input(), _ds );
			break;
		case Remove:
			result = QL::Remove( table, m.Input(), _ds );
			break;
		case Start:
			QL::Start( ms<MutationQL>(m), userPK );
			result = 1;
			break;
		case Stop:
			QL::Stop( ms<MutationQL>(m), userPK );
			result = 1;
			break;
//		default:
//			THROW( "unknown type" );
		}
		if( table.Name.starts_with("um_") )
			Try( [&]{UM::ApplyMutation( m, (UserPK)result.value_or(0) );} );
/*		sl _{ _applyMutationListenerMutex };
		//deprecated, use hooks
		if( var p = ApplyMutationListeners().find(string{table.Prefix()}); p!=ApplyMutationListeners().end() )
			std::for_each( p->second.begin(), p->second.end(), [&](var& f){f(m, result.value_or(0));} );
*/
		return *result;
	}

	α IntrospectFields( sv /*typeName*/, const DB::Table& mainTable, const TableQL& fieldTable, json& jData, const DB::Schema& schema )ε{
		auto fields = json::array();
		var pTypeTable = fieldTable.FindTable( "type" );
		var haveName = fieldTable.FindColumn( "name" )!=nullptr;
		var pOfTypeTable = pTypeTable->FindTable( "ofType" );
		json jTable;
		jTable["name"] = mainTable.JsonTypeName();

		auto addField = [&]( sv name, sv typeName, QLFieldKind typeKind, sv ofTypeName, optional<QLFieldKind> ofTypeKind ){
			json field;
			if( haveName )
				field["name"] = name;
			if( pTypeTable ){
				json type;
				auto setField = []( const TableQL& t, json& j, str key, sv x ){ if( t.FindColumn(key) ){ if(x.size()) j[key]=x; else j[key]=nullptr; } };
				auto setKind = []( const TableQL& t, json& j, optional<QLFieldKind> pKind ){
					if( t.FindColumn("kind") ){
						if( pKind )
							j["kind"] = (uint)*pKind;
						else
							j["kind"] = nullptr;
					}
				};
				setField( *pTypeTable, type, "name", typeName );
				setKind( *pTypeTable, type, typeKind );
				if( pOfTypeTable && (ofTypeName.size() || ofTypeKind) ){
					json ofType;
					setField( *pOfTypeTable, ofType, "name", ofTypeName );
					setKind( *pOfTypeTable, ofType, ofTypeKind );
					type["ofType"] = ofType;
				}
				field["type"] = type;
			}
			fields.push_back( field );
		};
		function<void(const DB::Table&, bool, string, const DB::Schema&)> addColumns = [&addColumns,&addField,&mainTable]( const DB::Table& dbTable, bool isMap, string prefix={}, const DB::Schema& schema ){
			for( var& column : dbTable.Columns ){
				BREAK_IF( column.Name=="member_id" );
				string fieldName;
				string qlTypeName;
				auto rootType{ QLFieldKind::Scalar };
				if( column.PKTable.empty() ){
					if( prefix.size() && column.Name=="id" )//use NID see RolePermission's permissions
						continue;
					fieldName = DB::Schema::ToJson( column.Name );
					qlTypeName = ColumnQL::QLType( column );//column.PKTable.empty() ? ColumnQL::QLType( column ) : dbTable.JsonTypeName();
				}
				else if( var pPKTable=schema.TryFindTable(column.PKTable); pPKTable ){
					auto pChildColumn = dbTable.ChildColumn();
					if( !isMap || pPKTable->IsFlags() || (pChildColumn && pChildColumn->Name==column.Name)  ){ //!RolePermission || right_id || um_groups.member_id
						if( find_if(dbTable.Columns, [&column](var& c){return c.QLAppend==DB::Schema::ToJson(column.Name);})!=dbTable.Columns.end() )
							continue;
						if( auto pExtendedFromTable = mainTable.GetExtendedFromTable(schema); pExtendedFromTable && mainTable.SurrogateKey().Name==column.Name ){//extension table
							addColumns( *pPKTable, false, prefix, schema );
							continue;
						}
						qlTypeName = pPKTable->JsonTypeName();
						if( pPKTable->IsFlags() ){
							fieldName = DB::Schema::ToPlural<sv>( fieldName );
							rootType = QLFieldKind::List;
						}
						else if( pChildColumn ){
							fieldName = DB::Schema::ToPlural<sv>( DB::Schema::ToJson(Str::Replace(column.Name, "_id", "")) );
							rootType = QLFieldKind::List;
						}
						else{
							fieldName = DB::Schema::ToJson<sv>( qlTypeName );
							rootType = pPKTable->IsEnum() ? QLFieldKind::Enum : QLFieldKind::Object;
						}
					}
					else{ //isMap
						//if( !typeName.starts_with(pPKTable->JsonTypeName()) )//typeName==RolePermission, don't want role columns, just permissions.
						addColumns( *pPKTable, false, pPKTable->JsonTypeName(), schema );
						continue;
					}
				}
				else
					THROW( "Could not find table '{}'", column.PKTable );

				auto pChildColumn = dbTable.ChildColumn();//group.member_id may not exist.
				var isNullable = pChildColumn || column.IsNullable;
				var typeName2 = isNullable ? qlTypeName : "";
				var typeKind = isNullable ? rootType : QLFieldKind::NonNull;
				var ofTypeName = isNullable ? "" : qlTypeName;
				var ofTypeKind = isNullable ? optional<QLFieldKind>{} : rootType;

				addField( fieldName, typeName2, typeKind, ofTypeName, ofTypeKind );
			}
		};
		addColumns( mainTable, mainTable.IsMap(schema), {}, schema );
		for( var& [name,pTable] : schema.Tables ){
			auto fnctn = [addField,pTable,&mainTable, &schema]( var& c1Name, var& c2Name ){
				if( var pColumn1=pTable->FindColumn(c1Name), pColumn2=pTable->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ ){
					if( pColumn1->PKTable==mainTable.Name ){
						var pTable2 = schema.Tables.at( pColumn2->PKTable );
						var jsonType = pTable->Columns.size()==2 ? pTable2->JsonTypeName() : pTable->JsonTypeName();
						addField( DB::Schema::ToPlural<string>(DB::Schema::ToJson<sv>(jsonType)), {}, QLFieldKind::List, jsonType, QLFieldKind::Object );
					}
				}
			};
			var child = pTable->ChildColumn();
			var parent = pTable->ParentColumn();
			if( child && parent ){
				fnctn( child->Name, parent->Name );
				fnctn( parent->Name, child->Name );
			}
		}
		jTable["fields"] = fields;
		jData["__type"] = jTable;
	}
	α IntrospectEnum( sv /*typeName*/, const DB::Table& baseTable, const TableQL& fieldTable, json& jData, sp<DB::IDataSource> ds )ε{
		var& dbTable = baseTable.QLView.empty() ? baseTable : ds->Schema().FindTable( baseTable.QLView );
		ostringstream sql{ "select ", std::ios::ate };
		vector<string> columns;
		for_each( fieldTable.Columns, [&columns, &dbTable](var& x){ if(dbTable.FindColumn(x.JsonName)) columns.push_back(x.JsonName);} );//sb only id/name.
		sql << Str::AddCommas( columns ) << " from " << dbTable.Name << " order by id";
		auto fields = json::array();
		ds->Select( sql.str(), [&]( const DB::IRow& row ){
			json j;
			for( uint i=0; i<columns.size(); ++i ){
				if( columns[i]=="id" )
					j[columns[i]] = row.GetUInt( i );
				else
					j[columns[i]] = row.GetString( i );
			}
			fields.push_back( j );
		} );
		json jTable;
		jTable["enumValues"] = fields;
		jData["__type"] = jTable;
	}
	α GraphQL::QueryType( const TableQL& typeTable, json& jData )ε->void{
		THROW_IF( typeTable.Args.find("name")==typeTable.Args.end(), "__type data for all names '{}' not supported", typeTable.JsonName );
		var typeName = typeTable.Args["name"].get<string>();
		auto p = find_if( _ds->Schema().Tables, [&](var& t){ return t.second->JsonTypeName()==typeName;} ); THROW_IF( p==_ds->Schema().Tables.end(), "Could not find table '{}' in schema", typeName );
		for( var& qlTable : typeTable.Tables ){
			if( qlTable.JsonName=="fields" ){
				if( var pObject = _introspection ? _introspection->Find(typeName) : nullptr; pObject )
					jData["__type"] = pObject->ToJson( qlTable );
				else
					IntrospectFields( typeName,  *p->second, qlTable, jData, _ds->Schema() );
			}
			else if( qlTable.JsonName=="enumValues" )
				IntrospectEnum( typeName,  *p->second, qlTable, jData, _ds );
			else
				THROW( "__type data for '{}' not supported", qlTable.JsonName );
		}
	}
	α QuerySchema( const TableQL& schemaTable, json& jData, sp<DB::IDataSource> ds )ε->void{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		var& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		auto fields = json::array();
		for( var& nameTablePtr : ds->Schema().Tables ){
			var pDBTable = nameTablePtr.second;
			var childColumn = pDBTable->ChildColumn();
			var jsonType = pDBTable->JsonTypeName();

			json field;
			field["name"] = Ƒ( "create{}"sv, jsonType );
			var addField = [&jsonType, pDBTable, &fields]( sv name, bool allColumns=false, bool idColumn=true ){
				json field;
				auto args = json::array();
				for( var& column : pDBTable->Columns ){
					if(   (column.Name=="id" && !idColumn) || (column.Name!="id" && !allColumns) )
						continue;
					json arg;
					arg["name"] = DB::Schema::ToJson( column.Name );
					arg["defaultValue"] = nullptr;
					json type; type["name"] = ColumnQL::QLType( column );
					arg["type"]=type;
					args.push_back( arg );
				}
				field["args"] = args;
				field["name"] = Ƒ( "{}{}", name, jsonType );
				fields.push_back( field );
			};
			if( !childColumn ){
				addField( "insert", true, false );
				addField( "update", true );

				addField( "delete" );
				addField( "restore" );
				addField( "purge" );
			}
			else{
				addField( "add", true, false );
				addField( "remove", true, false );
			}
		}
		json jmutationType;
		jmutationType["fields"] = fields;
		jmutationType["name"] = "Mutation";
		json jSchema; jSchema["mutationType"] = jmutationType;
		jData["__schema"] = jmutationType;
	}
#define TEST_ACCESS(a,b,c) Trace( _tags, "TEST_ACCESS({},{},{})", a, b, c )
	α GraphQL::QueryTable( const TableQL& table, UserPK userPK, json& jData )ε->void{
		TEST_ACCESS( "Read", table.DBName(), userPK ); //TODO implement.
		if( table.JsonName=="__type" )
			QueryType( table, jData );
		else if( table.JsonName=="__schema" )
			QuerySchema( table, jData, _ds );
		else
			QL::Query( table, jData, userPK, _ds );
	}

	α GraphQL::QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->json{
		json data;
		for( var& table : tables )
			QueryTable( table, userPK, data["data"] );
		return data;
	}

	α GraphQL::CoQuery( string q_, UserPK u_, SL sl )ι->TPoolAwait<json>{
		return Coroutine::TPoolAwait<json>( [q=move(q_), u=u_,this](){
			return mu<json>(Query(q,u));
		}, {}, sl );
	}

	α GraphQL::Query( sv query, UserPK userPK )ε->json{
		Trace( _tags, "{}", query );
		try{
			var qlType = ParseQL( query );
			vector<TableQL> tableQueries;
			json j;
			if( qlType.index()==1 ){
				var& mutation = get<MutationQL>( qlType );
				uint result = Mutation( mutation, userPK );
				str resultMemberName = mutation.Type==EMutationQL::Create ? "id" : "rowCount";
				var wantResults = mutation.ResultPtr && mutation.ResultPtr->Columns.size()>0;
				if( wantResults && mutation.ResultPtr->Columns.front().JsonName==resultMemberName )
					j["data"][mutation.JsonName][resultMemberName] = result;
				else if( mutation.ResultPtr )
					tableQueries.push_back( *mutation.ResultPtr );
			}
			else
				tableQueries = get<vector<TableQL>>( qlType );
			json y = tableQueries.size() ? QueryTables( tableQueries, userPK ) : j;
			//Dbg( y.dump() );
			return y;
		}
		catch( const nlohmann::json::exception& e ){
			THROW( "Error parsing query '{}'.  '{}'", query, e.what() );
		}
	}
}}