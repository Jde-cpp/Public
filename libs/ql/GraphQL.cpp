#include <jde/ql/GraphQL.h>
#include <jde/framework/str.h>
#include <jde/framework/io/json.h>
#include <jde/ql/GraphQLHook.h>
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/ql/Introspection.h>
#include <jde/db/meta/Column.h>
#include <jde/db/IRow.h>
#include <jde/db/Database.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/generators/WhereClause.h>
#include "../../../Framework/source/DateTime.h"
#include "GraphQuery.h"
#include "ops/Insert.h"
#include "ops/Purge.h"
#include "types/Parser.h"

#define let const auto
namespace Jde{
	using DB::EValue;
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
		for( let& [name,v] : schema.Types )
			_schema->Types.emplace( name, v );
		for( let& [name,v] : schema.Tables )
			_schema->Tables.emplace( name, v );
	}

	α DB::SetQLIntrospection( json&& j )ε->void{
		_introspection = mu<GraphQL::Introspection>( move(j) );
	}
*/
//namespace DB{
	α QL::ToJsonName( const DB::Column& c )ε->tuple<string,string>{
		string tableName;
		auto memberName{ DB::Schema::ToJson(c.Name) };
		if( c.IsFlags() || c.IsEnum() ){
			tableName = c.PKTable->Name;
			memberName = DB::Schema::ToJson( c.PKTable->NameWithoutType() );
			if( c.IsEnum() )
				memberName = DB::Schema::ToSingular( memberName );
		}
		return make_tuple( memberName, tableName );
	}
	α QL::ParseQL( sv query )ε->RequestQL{
		return Parse( query );
	}
namespace QL{
	α SubWhere( const DB::Table& table, const DB::Column& c, vector<DB::Value>& params, uint paramIndex )ε->string{
		std::ostringstream sql{ "=( select id from ", std::ios::ate }; sql << table.Name << " where " << c.Name;
		if( c.QLAppend.size() ){
			CHECK( paramIndex<params.size() && params[paramIndex].index()==(uint)EValue::String );
			let split = Str::Split( get<string>(params.back()), "\\" ); CHECK( split.size() );
			let appendColumnName = c.QLAppend;
			let pColumn = table.FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable );
			sql << (split.size()==1 ? " is null" : "=?") << " and " << appendColumnName << "=(select id from " <<  pColumn->PKTable << " where name=?) )";
			if( split.size()>1 ){
				params.push_back( string{split[1]} );
				params[paramIndex] = string{ split[0] };
			}
		}
		else
			sql << "=? )";
		return sql.str();
	}

	α Update( const DB::Table& table, const MutationQL& m, sp<DB::IDataSource> ds )->tuple<uint,DB::Value>{
		let pExtendedFromTable = table.GetExtendedFromTable();
		auto [count,rowId] = pExtendedFromTable ? Update(*pExtendedFromTable, m, ds) : make_tuple( 0, 0 );
		std::ostringstream sql{ Ƒ("update {} set ", table.Name), std::ios::ate };
		vector<DB::Value> parameters; parameters.reserve( table.Columns.size() );
		let input = m.Input();
		string sqlUpdate;
		DB::WhereClause where;
		for( let& column : table.Columns ){
			let& c = *column;
			if( !c.Updateable ){
				if( c.SKIndex ){
					if( pExtendedFromTable )
						where.Add( c.Name, rowId );
					else if( let pId = m.Args.find( DB::Schema::ToJson(c.Name) ); pId!=m.Args.end() )
						where.Add( c.Name, rowId=ToObject(EType::ULong, *pId, c.Name) );
					else{
						auto parentColumnName = table.ParentColumn() ? table.ParentColumn()->Name : string{};
						auto pTable = c.Name==parentColumnName ? table.ParentTable() : table.ChildTable(); CHECK( pTable );
						auto pValue = m.Args.find( "target" );
						sv cName{ pValue==m.Args.end() ? "name" : "target" };
						if( pValue==m.Args.end() ){
							pValue = m.Args.find( cName ); CHECK( pValue!=m.Args.end() );
						}
						let pNameColumn = pTable->FindColumn( cName ); CHECK( pNameColumn );
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
				let [memberName, tableName] = ToJsonName( c );
				let pValue = input.find( memberName );
				if( pValue==input.end() )
					continue;
				if( parameters.size() )
					sql << ", ";
				sql << c.Name << "=?";
				if( c.IsFlags() ){
					uint value = 0;
					if( pValue->is_array() && pValue->size() ){
						optional<flat_map<string,uint>> values;
						[] (auto& values, auto& tableName, sp<DB::IDataSource> ds)ι->Coroutine::Task {
							AwaitResult result = co_await ds->SelectMap<string,uint>(Ƒ("select name, id from {}", tableName));
							values = *( result.UP<flat_map<string,uint>>() );
						}(values, tableName, ds);
						while( !values )
							std::this_thread::yield();

						for( let& flag : *pValue ){
							if( let pFlag = values->find(flag); pFlag != values->end() )
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
			for( let& param : where.Parameters() ) parameters.push_back( param );
			if( sqlUpdate.size() )
				sql << sqlUpdate;
			sql << ' ' << where.Move();

			result = ds->Execute( sql.str(), parameters );
		}
		return make_tuple( count+result, rowId );
	}
	α SetDeleted( const DB::Table& table, uint id, iv value, sp<DB::IDataSource> ds )ε->uint{
		let& column{ table.GetColumn("deleted") };
		vector<DB::Value> parameters{ id };
		const string sql{ Ƒ("update {} set deleted={} where id=?", column.Table->DBName, value) };
		return ds->Execute( sql, parameters );
	}
	α Delete( const DB::Table& table, const MutationQL& m, sp<DB::IDataSource> ds )ε->uint{
		return SetDeleted( table, m.Id(), ds->Syntax().UtcNow(), ds );
	}
	α Restore( const DB::Table& table, const MutationQL& m, sp<DB::IDataSource> ds )ε->uint{
		return SetDeleted( table, m.Id(), "null", ds );
	}

	α ChildParentParams( sv childId, sv parentId, const json& input )->vector<DB::Value>{
		vector<DB::Value> parameters;
		if( let p = input.find(childId); p!=input.end() )
			parameters.push_back( ToObject(EType::ULong, *p, childId) );
		else if( let p = input.find("target"); p!=input.end() )
			parameters.push_back( p->get<string>() );
		else
			THROW( "Could not find '{}' or target in '{}'", childId, input.dump() );

		if( let p = input.find(parentId); p!=input.end() )
			parameters.push_back( ToObject(EType::ULong, *p, parentId) );
		else if( let p = input.find("name"); p!=input.end() )
			parameters.push_back( p->get<string>() );
		else
			THROW( "Could not find '{}' or name in '{}'", parentId, input.dump() );

		return parameters;
	};
	α Add( const DB::Table& table, const json& input, sp<DB::IDataSource> ds )->uint{
		string childColName = table.ChildColumn()->Name;
		string parentColName = table.ParentColumn()->Name;
		std::ostringstream sql{ "insert into ", std::ios::ate }; sql << table.Name << "(" << childColName << "," << parentColName;
		let childId = DB::Schema::ToJson( childColName );
		let parentId = DB::Schema::ToJson( parentColName );
		auto parameters = ChildParentParams( childId, parentId, input );
		std::ostringstream values{ "?,?", std::ios::ate };
		for( let& [name,value] : input.items() ){
			if( name==childId || name==parentId )
				continue;
			let columnName = DB::Schema::FromJson( name );
			auto pColumn = table.FindColumn( columnName ); THROW_IF(!pColumn, "Could not find column {}.{}.", table.Name, columnName );
			sql << "," << columnName;
			values << ",?";

			parameters.push_back( ToObject(pColumn->Type, value, name) );
		}
		sql << ")values( " << values.str() << ")";
		return ds->Execute( sql.str(), parameters );
	}
	α Remove( const DB::Table& table, const json& input, sp<DB::IDataSource> ds )->uint{
		let& childId = table.ChildColumn()->Name;
		let& parentId = table.ParentColumn()->Name;
		auto params = ChildParentParams( DB::Schema::ToJson(childId), DB::Schema::ToJson(parentId), input );
		std::ostringstream sql{ "delete from ", std::ios::ate }; sql << table.Name << " where " << childId;
		if( (EValue)params[0].index()==EValue::String ){
			let pTable = table.ChildTable(); CHECK( pTable );
			let pTarget = pTable->FindColumn( "target" ); CHECK( pTarget );
			sql << SubWhere( *pTable, *pTarget, params, 0 );
		}
		else
			sql << "=?";
		sql << " and " << parentId;
		if( (EValue)params[1].index()==EEValue:String ){
			let pTable = table.ParentTable(); CHECK( pTable );
			let pName = pTable->FindColumn( "name" ); CHECK( pName );
			sql << SubWhere( *pTable, *pName, params, 1 );
/*			sql << "=( select id from " << pTable->Name << " where name";
			if( pName->QLAppend.size() )
			{

				let split = Str::Split( get<string>(params[1]), "\\" ); CHECK( split.size() );
				let appendColumnName = DB::Schema::FromJson( pName->QLAppend );
				let pColumn = pTable->FindColumn( appendColumnName ); CHECK( pColumn ); CHECK( pColumn->PKTable.size() );
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
		let& table = _schema->FindTable( m.TableName() );
		if( let pAuthorizer = UM::FindAuthorizer(table.Name); pAuthorizer )
			pAuthorizer->Test( m.Type, userPK );
		optional<uint> result;
		switch( m.Type ){
		//using namespace DB;
		using enum EMutationQL;
		case Create:{
			optional<uint> extendedFromId;
			if( let pExtendedFrom = table.GetExtendedFromTable(); pExtendedFrom ){
				[](auto extendedFromId, let& pExtendedFrom, let& m, auto userPK, auto ds)ι->Task {
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
		if( let p = ApplyMutationListeners().find(string{table.Prefix()}); p!=ApplyMutationListeners().end() )
			std::for_each( p->second.begin(), p->second.end(), [&](let& f){f(m, result.value_or(0));} );
*/
		return *result;
	}

	α IntrospectFields( sv /*typeName*/, const DB::Table& mainTable, const TableQL& fieldTable, json& jData, const DB::Schema& schema )ε{
		auto fields = json::array();
		let pTypeTable = fieldTable.FindTable( "type" );
		let haveName = fieldTable.FindColumn( "name" )!=nullptr;
		let pOfTypeTable = pTypeTable->FindTable( "ofType" );
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
		function<void(const DB::Table&, bool, string)> addColumns = [&addColumns,&addField,&mainTable]( const DB::Table& dbTable, bool isMap, string prefix={} ){
			for( let& c : dbTable.Columns ){
				let& column = *c;
				BREAK_IF( column.Name=="member_id" );
				string fieldName;
				string qlTypeName;
				auto rootType{ QLFieldKind::Scalar };
				if( !column.PKTable ){
					if( prefix.size() && column.SKIndex )//use NID see RolePermission's permissions
						continue;
					fieldName = DB::Schema::ToJson( column.Name );
					qlTypeName = ColumnQL::QLType( column );//column.PKTable.empty() ? ColumnQL::QLType( column ) : dbTable.JsonTypeName();
				}
				else if( column.PKTable ){
					auto pChildColumn = dbTable.ChildColumn();
					if( !isMap || column.PKTable->IsFlags || (pChildColumn && pChildColumn->Name==column.Name)  ){ //!RolePermission || right_id || um_groups.member_id
						if( find_if(dbTable.Columns, [&column](let& c){return c->QLAppend==column.Name;})!=dbTable.Columns.end() )
							continue;
						if( auto pExtendedFromTable = mainTable.GetExtendedFromTable(); pExtendedFromTable && mainTable.GetPK().Name==column.Name ){//extension table
							addColumns( *column.PKTable, false, prefix );
							continue;
						}
						qlTypeName = column.PKTable->JsonTypeName();
						if( column.PKTable->IsFlags ){
							fieldName = DB::Schema::ToPlural<sv>( fieldName );
							rootType = QLFieldKind::List;
						}
						else if( pChildColumn ){
							fieldName = DB::Schema::ToPlural<sv>( DB::Schema::ToJson(Str::Replace(column.Name, "_id", "")) );
							rootType = QLFieldKind::List;
						}
						else{
							fieldName = DB::Schema::ToJson<sv>( qlTypeName );
							rootType = column.PKTable->IsEnum() ? QLFieldKind::Enum : QLFieldKind::Object;
						}
					}
					else{ //isMap
						//if( !typeName.starts_with(pPKTable->JsonTypeName()) )//typeName==RolePermission, don't want role columns, just permissions.
						addColumns( *column.PKTable, false, column.PKTable->JsonTypeName() );
						continue;
					}
				}
				else
					THROW( "[{}]Could not find table.", column.PKTable->Name );

				auto pChildColumn = dbTable.ChildColumn();//group.member_id may not exist.
				let isNullable = pChildColumn || column.IsNullable;
				let typeName2 = isNullable ? qlTypeName : "";
				let typeKind = isNullable ? rootType : QLFieldKind::NonNull;
				let ofTypeName = isNullable ? "" : qlTypeName;
				let ofTypeKind = isNullable ? optional<QLFieldKind>{} : rootType;

				addField( fieldName, typeName2, typeKind, ofTypeName, ofTypeKind );
			}
		};
		addColumns( mainTable, mainTable.IsMap(), {} );
		for( let& [name,pTable] : schema.Tables ){
			auto fnctn = [addField,pTable,&mainTable, &schema]( let& c1Name, let& c2Name ){
				if( let pColumn1=pTable->FindColumn(c1Name), pColumn2=pTable->FindColumn(c2Name) ; pColumn1 && pColumn2 /*&& pColumn->PKTable==n*/ ){
					if( pColumn1->PKTable->Name==mainTable.Name ){
						let pTable2 = pColumn2->PKTable;
						let jsonType = pTable->Columns.size()==2 ? pTable2->JsonTypeName() : pTable->JsonTypeName();
						addField( DB::Schema::ToPlural<string>(DB::Schema::ToJson<sv>(jsonType)), {}, QLFieldKind::List, jsonType, QLFieldKind::Object );
					}
				}
			};
			let child = pTable->ChildColumn();
			let parent = pTable->ParentColumn();
			if( child && parent ){
				fnctn( child->Name, parent->Name );
				fnctn( parent->Name, child->Name );
			}
		}
		jTable["fields"] = fields;
		jData["__type"] = jTable;
	}
	α IntrospectEnum( sv /*typeName*/, const DB::Table& baseTable, const TableQL& fieldTable, json& jData, sp<DB::IDataSource> ds )ε{
		const DB::View& dbTable = baseTable.QLView ? *baseTable.QLView : dynamic_cast<const DB::View&>(baseTable);
		std::ostringstream sql{ "select ", std::ios::ate };
		vector<string> columns;
		for_each( fieldTable.Columns, [&columns, &dbTable](let& x){ if(dbTable.FindColumn(x.JsonName)) columns.push_back(x.JsonName);} );//sb only id/name.
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
		let typeName = typeTable.Args["name"].get<string>();
		auto p = find_if( _schema->Tables, [&](let& t){ return t.second->JsonTypeName()==typeName;} ); THROW_IF( p==_schema->Tables.end(), "Could not find table '{}' in schema", typeName );
		for( let& qlTable : typeTable.Tables ){
			if( qlTable.JsonName=="fields" ){
				if( let pObject = _introspection ? _introspection->Find(typeName) : nullptr; pObject )
					jData["__type"] = pObject->ToJson( qlTable );
				else
					IntrospectFields( typeName,  *p->second, qlTable, jData, *_schema );
			}
			else if( qlTable.JsonName=="enumValues" )
				IntrospectEnum( typeName,  *p->second, qlTable, jData, _ds );
			else
				THROW( "__type data for '{}' not supported", qlTable.JsonName );
		}
	}
	α QuerySchema( const TableQL& schemaTable, json& jData, const DB::Schema& schema )ε->void{
		THROW_IF( schemaTable.Tables.size()!=1, "Only Expected 1 table type for __schema {}", schemaTable.Tables.size() );
		let& mutationTable = schemaTable.Tables[0]; THROW_IF( mutationTable.JsonName!="mutationType", "Only mutationType implemented for __schema - {}", mutationTable.JsonName );
		auto fields = json::array();
		for( let& nameTablePtr : schema.Tables ){
			let pDBTable = nameTablePtr.second;
			let childColumn = pDBTable->ChildColumn();
			let jsonType = pDBTable->JsonTypeName();

			json field;
			field["name"] = Ƒ( "create{}"sv, jsonType );
			let addField = [&jsonType, pDBTable, &fields]( sv name, bool allColumns=false, bool idColumn=true ){
				json field;
				auto args = json::array();
				for( let& column : pDBTable->Columns ){
					if(   (column->IsPK() && !idColumn) || (!column->IsPK() && !allColumns) )
						continue;
					json arg;
					arg["name"] = DB::Schema::ToJson( column->Name );
					arg["defaultValue"] = nullptr;
					json type; type["name"] = ColumnQL::QLType( *column );
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
			QuerySchema( table, jData, *_schema );
		else
			QL::Query( table, jData, userPK, _ds );
	}

	α GraphQL::QueryTables( const vector<TableQL>& tables, UserPK userPK )ε->json{
		json data;
		for( let& table : tables )
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
		let qlType = ParseQL( query );
		vector<TableQL> tableQueries;
		json j;
		if( qlType.index()==1 ){
			let& mutation = get<MutationQL>( qlType );
			uint result = Mutation( mutation, userPK );
			str resultMemberName = mutation.Type==EMutationQL::Create ? "id" : "rowCount";
			let wantResults = mutation.ResultPtr && mutation.ResultPtr->Columns.size()>0;
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
}}