#include "SchemaDdl.h"
#include "TableDdl.h"
#include "Syntax.h"
#include <jde/db/DataSource.h>
#include <jde/db/metadata/Column.h>
#include <jde/io/File.h>
#include "Procedure.h"

#define let const auto
namespace Jde::DB{
	constexpr ELogTags _tags{ ELogTags::Sql };
	α UniqueIndexName( const Index& index, bool uniqueName, const vector<Index>& indexes )ε->string;
	α GetData( const jobject& j )ε->jarray;
	α GetFlagsData( const jobject& j )ε->flat_map<uint,string>;
//todo doc relativeScriptPath
//todo add db version table.
	α SchemaDdl::Sync( const Schema& config, const jobject& jconfig, const fs::path& relativeScriptPath )ε->void{
		SyncTables( config );
		SyncScripts( config, jconfig, relativeScriptPath );
		SyncData( config, jconfig );
		SyncForeignKeys( config );
	}

	α SchemaDdl::SyncData( const Schema& config, const jobject& jconfig )ε->void{
		IDataSource& ds = *DS();
		for( let& [tableName, table] : config.Tables ){
			const jobject& jTable = jconfig.at( tableName ).as_object();

			for( let& [id,value] : GetFlagsData(jTable) ){
				if( ds.Scaler<uint>( Ƒ("select count(*) from {} where id=?", table->DBName), {id})==0 )
					ds.Execute( Ƒ("insert into {}(id,name)values( ?, ? )", table->DBName), {id, value} );
			}

			for( let& j : GetData(jTable) ){
				let& jData = j.as_object();
				vector<object> params;

				std::ostringstream osSelect{ "select count(*) from ", std::ios::ate }; osSelect << tableName << " where ";
				vector<object> selectParams;
				std::ostringstream osWhere;
				let set = [&,&table=*table]() {
					osWhere.str("");
					for( let& keyColumn : table.SurrogateKeys ){
						if( osWhere.tellp() != std::streampos(0) )
							osWhere << " and ";
						osWhere << keyColumn << "=?";
						if( let pData = jData.find( Schema::ToJson(keyColumn) ); pData!=jData.end() )
							selectParams.push_back( ToObject(table.FindColumn(keyColumn)->Type, pData->value(), keyColumn) );
						else{
							selectParams.clear();
							break;
						}
					}
					return selectParams.size();
				};
				try{
					if( !set() )
						for( auto p = table->NaturalKeys.begin(); p!=table->NaturalKeys.end() && !set(); ++p );
				}
				catch( std::exception& e ){
					throw Exception{ SRCE_CUR, move(e), "Could not set data for {}", tableName };
				}
				THROW_IF( selectParams.empty(), "Could not find keys in data for '{}'", tableName );
				osSelect << osWhere.str();

				std::ostringstream osInsertValues;
				std::ostringstream osInsertColumns;
				for( let& column : table->Columns ){
					let jsonName = Schema::ToJson( column->Name );
					let pData = jData.find( jsonName );
					let haveData = pData!=jData.end();
					if( !haveData && column->Default.empty() )
						continue;

					if( params.size() ){
						osInsertValues << ",";
						osInsertColumns << ",";
					}
					osInsertColumns << column->Name;
					if( haveData ){
						osInsertValues << "?";
						params.push_back( ToObject(column->Type, pData->value(), column->Name) );
					}
					else
						osInsertValues << ( column->Default=="$now" ? ToStr(Syntax().UtcNow()) : column->Default );
				}
				if( ds.Scaler<uint>( osSelect.str(), selectParams)==0 ){
					std::ostringstream sql;
					let identityInsert = table->HaveSequence() && Syntax().NeedsIdentityInsert();
					if( identityInsert )
						sql << "SET IDENTITY_INSERT " << tableName << " ON;" << std::endl;
					sql << Ƒ( "insert into {}({})values({})", tableName, osInsertColumns.str(), osInsertValues.str() );
					if( identityInsert )
						sql << std::endl << "SET IDENTITY_INSERT " << tableName << " OFF;";
					ds.Execute( sql.str(), params );
				}
			}
		}
	}

	α SchemaDdl::SyncForeignKeys( const Schema& config )ε->void{
		for( auto& [tableName, table] : config.Tables ){
			for( auto& column : table->Columns ){
				if( column->PKTable.empty() )
					continue;
				if( find_if(ForeignKeys, [&,t=tableName](let& fk){return fk.second.Table==t && fk.second.Columns==vector<string>{column->Name};})!=ForeignKeys.end() )
					continue;
				let pPKTable = schema.Tables.find( Schema::FromJson(column->PKTable) );
				if( pPKTable == schema.Tables.end() ){
					ERR( "Could not find primary key table '{}' for {}.{}", Schema::FromJson(column->PKTable), tableName, column->Name );
					continue;
				}
				if( pPKTable->second->FlagsData.size() )
					column->IsFlags = true;
				else{
					if( pPKTable->second->Data.size() )
						column->IsEnum = true;

					auto i = 0;
					auto getName = [&, &t=tableName](auto i)->string{//&t for clang
						return Ƒ( "{}_{}{}_fk", AbbrevName(t), AbbrevName(pPKTable->first), i==0 ? "" : ToString(i) );
					};
					auto name = getName( i++ );
					for( ; ForeignKeys.find(name)!=ForeignKeys.end(); name = getName(i++) );

					let createStatement = ForeignKey::Create( name, column->Name, *pPKTable->second, tableName );
					ForeignKeys.emplace( name, ForeignKey{name, tableName, {column->Name}, pPKTable->first} );
					ds.Execute( createStatement );
					Information{ _tags, "Created fk '{}'.", name };
				}
			}
		}
	}
	α SchemaDdl::SyncScripts(const Schema& config, const jobject& jconfig, const fs::path& relativeScriptPath )->void{
		const fs::path scriptRoot{ Settings::Env("db/scriptDir").value_or(relativeScriptPath.string()) };
		auto scripts = jconfig.contains("scripts") ? jconfig.at("scripts").as_array() : jarray{};
		for( let& script : scripts ){
			auto scriptFile = fs::path{ string{script.as_string()} };
			let procName = scriptFile.stem();
			if( Procedures.find(procName.string())!=Procedures.end() )
				continue;
			if( Syntax()->ProcFileSuffix().size() )
				scriptFile = scriptFile.parent_path()/( scriptFile.stem().string()+string{Syntax()->ProcFileSuffix()}+scriptFile.extension().string() );
			let path = scriptRoot/scriptFile;
			let text = IO::FileUtilities::Load( path );
			Trace( _tags, "Executing '{}'", path.string() );
			let queries = Str::Split<sv,iv>( text, "\ngo"_iv );
			for( let& query : queries ){
				std::ostringstream os;
				for( uint i=0; i<query.size(); ++i ){
					if( query[i]=='#' )
						for( ; i<query.size() && query[i]!='\n'; ++i );
					if( i<query.size() )
						os.put( query[i] );
				}
				ds.Execute( os.str(), nullptr, nullptr, false ); //TODO! - add views for app server. ??
			}
			Information{ _tags, "Finished '{}'", path.string() };
		}
	}

	α SchemaDdl::SyncTables( const Schema& config )ε->void{
		const DB::Syntax& syntax = Syntax();
		IDataSource& ds = *DS();
		for( let& [tableName, table] : config.Tables ){
			sp<TableDdl> dbTable;
			if( let kv=Tables.find(tableName); kv!=Tables.end() ){
				dbTable = std::dynamic_pointer_cast<TableDdl>( kv->second );
				for( auto& column : table->Columns ){
					let pDBColumn = dbTable->FindColumn( column->Name ); if( !pDBColumn ){ Error{_tags,"Could not find db column {}.{}", tableName, column->Name}; continue; }
					if( pDBColumn->Default.empty() && column->Default.size() && column->Default!="$now" ){
						if( column->Type==DB::EType::Bit )
							ds.TryExecute( syntax.AddDefault(tableName, column->Name, column->Default=="true") );//TODO make default a DB::object.
						else
							ds.TryExecute( syntax.AddDefault(tableName, column->Name, column->Default) );
					}
				}
			}
			else{
				dbTable = ms<TableDdl>( *table );
				ds.Execute( dbTable->CreateText() );
				Information{ _tags, "Created table '{}'.", tableName };
				dbTable->Initialize( shared_from_this(), dbTable );
				if( table->HaveSequence() )
					dbTable->Indexes = ds.SchemaProc().LoadIndexes( dbTable->Name );
				Tables.emplace( tableName, dbTable );
			}

			for( let& index : Index::GetConfig(*table) ){
				auto& dbIndexes = dbTable->Indexes;
				if( find_if(dbIndexes, [&](let& db){ return db.TableName==index.TableName && db.Columns==index.Columns;} )!=dbIndexes.end() )
					continue;
				let name = UniqueIndexName( index, syntax.UniqueIndexNames(), dbIndexes );
				let indexCreate = index.Create( name, tableName, syntax );
				ds.Execute( indexCreate );
				dbIndexes.push_back( Index{name, tableName, index} );
				Information{ _tags, "Created index '{}.{}'.", tableName, name };
			}
			if( let procName = table->InsertProcName(); procName.size() && Procedures.find(procName)==Procedures.end() ){
				ds.Execute( dbTable->InsertProcText() );
				Procedures.emplace( procName, Procedure{procName} );
				Information{ _tags, "Created proc '{}'.", table->InsertProcName() };
			}
		}
	}

	α GetData( const jobject& j )ε->jarray{
		jarray data;
		if( auto kv = j.find("data"); kv!=j.end() ){
			uint id = 0;
			for( let& row : kv->value().as_array() ){
				if( row.is_string() ){
					json jRow;
					jRow["id"] = id++;
					jRow["name"] = row.as_string();
					data.push_back( jRow );
				}
				else
					data.push_back( row );
			}
		}
		return data;
	}

	α GetFlagsData( const jobject& j )ε->flat_map<uint,string>{
		flat_map<uint,string> flagsData;
		if( auto kv = j.find("flagsData"); kv!=j.end() ){
			uint i=0;
			for( let& col : kv->value().as_array() )
				flagsData.emplace( 1 << i++, col.as_string() );
		}
		return flagsData;
	}

	α UniqueIndexName( const DB::Index& index, bool uniqueName, const vector<Index>& indexes )ε->string{
		auto indexName=index.Name;
		bool checkOnlyTable = !index.PrimaryKey && !uniqueName;
		for( uint i=2; ; indexName = Ƒ( "{}{}", index.Name, i++ ) ){
			if( find_if(indexes.begin(), [&](let& x){ return x.Name==String(indexName) && (!checkOnlyTable || index.TableName==x.TableName);})==indexes.end() )
				break;
		}
		return indexName;
	}
}