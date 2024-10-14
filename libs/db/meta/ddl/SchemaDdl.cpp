#include "SchemaDdl.h"
#include "TableDdl.h"
#include <jde/db/generators/Syntax.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Catalog.h>
#include <jde/db/meta/Cluster.h>
#include <jde/db/meta/Column.h>
#include <jde/framework/io/File.h>
#include "../IServerMeta.h"
#include "Index.h"
#include "Procedure.h"


#define let const auto
namespace Jde::DB{
	constexpr ELogTags _tags{ ELogTags::Sql };

	SchemaDdl::SchemaDdl( sv name, const IServerMeta& loader )ε:
		Schema{ name, loader.LoadTables() },
		Procs{ loader.LoadProcs() },
		FKs{ loader.LoadForeignKeys() }
	{}

	α AbbrevName( sv schemaName )ι->string;
	α GetData( const jobject& j )ε->jarray;
	α Exists( const Schema& config )ι->bool;
	α GetFlagsData( const jobject& j )ε->flat_map<uint,string>;
	α UniqueIndexName( const Index& index, bool uniqueName, const vector<Index>& indexes )ε->string;

	α ConfigurationJson( const Schema& config )ε->const jobject&{
		return Settings::AsObject( Ƒ("dbServers/{}/catalogs/{}/schemas/{}/{}", config.Catalog->Cluster->ConfigName, config.Catalog->Name, config.DBName, config.Name) );
	}
//todo doc relativeScriptPath
//todo add db version table.
	α SchemaDdl::Sync( const Schema& config )ε->void{
		if( !Exists( config ) )
			Create( config );
		let& jconfig = ConfigurationJson(config );
		SchemaDdl db{ config.DBName, config.DS()->ServerMeta() };

		db.SyncTables( config );
		db.SyncScripts( config, jconfig );
		db.SyncData( config, jconfig );
		db.SyncFKs( config );
	}

	α SchemaDdl::SyncData( const Schema& config, const jobject& jconfig )ε->void{
		IDataSource& ds = *DS();
		for( let& [tableName, table] : config.Tables ){
			const jobject& jTable = jconfig.at( tableName ).as_object();

			for( let& [id,value] : GetFlagsData(jTable) ){
				if( ds.Scaler<uint>( Ƒ("select count(*) from {} where id=?", table->DBName), {Value{id}})==0 )
					ds.Execute( Ƒ("insert into {}(id,name)values( ?, ? )", table->DBName), {Value{id}, Value{value}} );
			}

			for( let& j : GetData(jTable) ){
				let& jData = j.as_object();
				vector<Value> params;

				std::ostringstream osSelect{ "select count(*) from ", std::ios::ate }; osSelect << tableName << " where ";
				vector<Value> selectParams;
				std::ostringstream osWhere;
				let set = [&,&table=*table]() {
					osWhere.str("");
					for( let& keyColumn : table.SurrogateKeys ){
						if( osWhere.tellp() != std::streampos(0) )
							osWhere << " and ";
						osWhere << keyColumn->Name << "=?";
						if( let pData = jData.find( Schema::ToJson(keyColumn->Name) ); pData!=jData.end() )
							selectParams.push_back( Value{keyColumn->Type, pData->value()} );
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
					if( !haveData && !column->Default )
						continue;

					if( params.size() ){
						osInsertValues << ",";
						osInsertColumns << ",";
					}
					osInsertColumns << column->Name;
					if( haveData ){
						osInsertValues << "?";
						params.push_back( Value(column->Type, pData->value()) );
					}
					else
						osInsertValues << ( column->Default->is_string() && column->Default->get_string()=="$now" ? ToStr(Syntax().UtcNow()) : column->Default->get_string() );
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

	α SchemaDdl::SyncFKs( const Schema& config )ε->void{
		for( auto& [tableName, table] : config.Tables ){
			for( auto& column : table->Columns ){
				if( !column->PKTable )
					continue;
				if( find_if(FKs, [&,t=tableName](let& fk){return fk.second.Table==t && fk.second.Columns==vector<string>{column->Name};})!=FKs.end() )
					continue;
				let pPKTable = column->PKTable;
				if( !column->IsFlags() ){
					auto getName = [&, &t=tableName](auto i)->string{//&t for clang
						return Ƒ( "{}_{}{}_fk", AbbrevName(t), AbbrevName(pPKTable->Name), i==0 ? "" : std::to_string(i) );
					};
					uint i{};
					auto name = getName( i++ );
					for( ; FKs.find(name)!=FKs.end(); name = getName(i++) );

					let createStatement = ForeignKey::Create( name, column->Name, *pPKTable, *table );
					config.DS()->Execute( createStatement );
					FKs.emplace( name, ForeignKey{name, tableName, {column->Name}, pPKTable->Name} );
					Information{ _tags, "Created fk '{}'.", name };
				}
			}
		}
	}
	α SchemaDdl::SyncScripts( const Schema& config, const jobject& jconfig )->void{
		const fs::path scriptRoot{ Settings::FindSV("dbServers/scriptDir").value_or("./") };
		auto scripts = jconfig.contains("scripts") ? jconfig.at("scripts").as_array() : jarray{};
		for( let& script : scripts ){
			auto scriptFile = fs::path{ string{script.as_string()} };
			let procName = scriptFile.stem();
			if( Procs.find(procName.string())!=Procs.end() )
				continue;
			if( Syntax().ProcFileSuffix().size() )
				scriptFile = scriptFile.parent_path()/( scriptFile.stem().string()+string{Syntax().ProcFileSuffix()}+scriptFile.extension().string() );
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
				config.DS()->Execute( os.str(), nullptr, nullptr, false ); //TODO! - add views for app server. ??
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
					if( pDBColumn->Default && pDBColumn->Default->is_string() && pDBColumn->Default->get_string()!="$now" )
						ds.TryExecute( syntax.AddDefault(tableName, column->Name, *pDBColumn->Default) );
				}
			}
			else{
				dbTable = ms<TableDdl>( *table );
				ds.Execute( dbTable->CreateStatement() );
				Information{ _tags, "Created table '{}'.", tableName };
				dbTable->Initialize( shared_from_this(), dbTable );
				if( table->HaveSequence() )
					dbTable->Indexes = ds.ServerMeta().LoadIndexes( dbTable->Name );
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
			if( let procName = table->InsertProcName(); procName.size() && Procs.find(procName)==Procs.end() ){
				ds.Execute( dbTable->InsertProcCreateStatement() );
				Procs.emplace( procName, Procedure{procName} );
				Information{ _tags, "Created proc '{}'.", table->InsertProcName() };
			}
		}
	}

	α SchemaDdl::Create( const Schema& config )ε->void{
		config.Catalog->DS()->Execute( Ƒ("CREATE SCHEMA {}", config.DBName) );
	}
#ifndef PROD
	α DropObjects( const Schema& config )ε->void{
		auto& ds = *config.DS();

		for( auto& [name, fk] : config.DS()->ServerMeta().LoadForeignKeys() ){
			if( find_if(config.Tables, [&fk](let& t){return t.second->DBName==fk.Table;})!=config.Tables.end() )
				ds.Execute( Ƒ("ALTER TABLE {} DROP CONSTRAINT {}", fk.Table, name) );
		}

		for( let& [tableName, table] : config.Tables ){
			if( table->InsertProcName().size() )
				ds.Execute( Ƒ("DROP PROCEDURE IF EXISTS {}", table->InsertProcName()) );
			if( table->PurgeProcName.size() )
				ds.Execute( Ƒ("DROP PROCEDURE IF EXISTS {}", table->PurgeProcName) );
			ds.Execute( Ƒ("DROP TABLE IF EXISTS {}", table->DBName) );
		}
		let& jconfig = ConfigurationJson( config );
		if( auto script = Json::FindString(jconfig, "meta"); script ){
			auto name = fs::path{ *script }.stem().string();
			let type = name.ends_with("_ql") ? "VIEW" : "PROCEDURE";
			ds.Execute( Ƒ("DROP {} IF EXISTS {}", type, name) );
		}
	}

	α SchemaDdl::Drop( const Schema& config )ε->void{
		if( Exists(config) ){
			if( !config.DS()->Syntax().SchemaDropsObjects() )
				DropObjects( config );
			config.DS()->Execute( Ƒ("DROP SCHEMA {}", config.DBName) );
		}
	}
#endif

	α AbbrevName( sv schemaName )ι->string{
		auto fnctn = []( let& word )->string {
			std::ostringstream os;
			for( let ch : word ){
				if( (ch!='a' && ch!='e' && ch!='i' && ch!='o' && ch!='u') || os.tellp() == std::streampos(0) )
					os << ch;
			}
			return word.size()>2 && os.str().size()<word.size()-1 ? os.str() : string{ word };
		};
		let singular = DB::Schema::ToSingular( schemaName );
		let splits = Str::Split( singular, '_' );
		std::ostringstream name;
		for( uint i=1; i<splits.size(); ++i ){
			if( i>1 )
				name << '_';
			name << fnctn( splits[i] );
		}
		return name.str();
	}

	α Exists( const Schema& config )ι->bool{
		return config.Catalog->DS()->Scaler<string>("SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?", {Value{config.DBName}}).has_value();
	}

	α GetData( const jobject& j )ε->jarray{
		jarray data;
		if( auto kv = j.find("data"); kv!=j.end() ){
			uint id = 0;
			for( let& row : kv->value().as_array() ){
				if( row.is_string() ){
					jobject jRow;
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
			if( find_if(indexes, [&](let& x){ return ToIV(x.Name)==ToIV(indexName) && (!checkOnlyTable || index.TableName==x.TableName);})==indexes.end() )
				break;
		}
		return indexName;
	}
}