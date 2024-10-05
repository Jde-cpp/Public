#include <jde/db/Database.h>
#include "c_api.h"
#include <jde/db/DataSource.h>
#include <jde/App.h>
#include <jde/io/File.h>
#include <jde/io/Json.h>
#include "../../../Framework/source/Settings.h"
#include <jde/Dll.h>
#include <jde/db/syntax/Syntax.h>
#include <boost/pointer_cast.hpp>

#define var const auto
namespace Jde{
	using nlohmann::ordered_json;
	static var _logTag{ Logging::Tag("sql") };
	α DB::SqlTag()ι->sp<LogTag>{ return _logTag; }

	α DB::Driver()ι->string{
		auto driver = Settings::Env( "db/driver" );
		if( driver && driver->empty() )
		  driver = nullopt;
		return driver.value_or( _msvc ? "Jde.DB.Odbc.dll" : "./libJde.MySql.so" );
	}

	α DB::ToParamString( uint c )->string{
		string y{'?'}; y.reserve( c*2-1 );
		for( uint i=1; i<c; ++i )
			y+=",?";
		return y;
	}
	α DB::LogDisplay( sv sql, const vector<object>* pParameters, string error )ι->string{
		ostringstream os;
		if( error.size() )
			os << move(error) << std::endl;
		uint prevIndex=0;
		for( uint sqlIndex=0, paramIndex=0, size = pParameters ? pParameters->size() : 0; (sqlIndex=sql.find_first_of('?', prevIndex))!=string::npos && paramIndex<size; ++paramIndex, prevIndex=sqlIndex+1 ){
			os << sql.substr( prevIndex, sqlIndex-prevIndex );
			var& o = (*pParameters)[paramIndex];
			if( (EObject)o.index()==EObject::String )
				os << "'";
			os << DB::ToString( o );
			if( (EObject)o.index()==EObject::String )
				os << "'";
		}
		if( prevIndex<sql.size() )
			os << sql.substr( prevIndex );
		return os.str();
	}
	α DB::Log( sv sql, const vector<object>* pParameters, SL sl )ι->void{
		Trace{ sl, ELogTags::Sql, "{}", LogDisplay(sql, pParameters, {}) };
	}

	α DB::Log( sv sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )ι->void{
		Log( level, ELogTags::Sql, sl, "{}", LogDisplay(sql, pParameters, error) );
	}

	α DB::LogNoServer( string sql, const vector<object>* pParameters, ELogLevel level, string error, SL sl )ι->void{
		Log( level, ELogTags::Sql | ELogTags::ExternalLogger, sl, "{}", LogDisplay(sql, pParameters, error) );
	}

	class DataSourceApi{
		DllHelper _dll;
	public:
		DataSourceApi( const fs::path& path ):
			_dll{ path },
			GetDataSourceFunction{ _dll["GetDataSource"] }
		{}
		decltype(GetDataSource) *GetDataSourceFunction;

		α Emplace( str connectionString )->sp<DB::IDataSource>{
			std::unique_lock l{ _connectionsMutex };
			auto pDataSource = _pConnections->find( connectionString );
			if( pDataSource == _pConnections->end() ){
				auto pNew = sp<Jde::DB::IDataSource>{ GetDataSourceFunction() };
				pNew->SetConnectionString( connectionString );
				pDataSource = _pConnections->emplace( connectionString, pNew ).first;
			}
			return pDataSource->second;
		}
		static sp<flat_map<string,sp<Jde::DB::IDataSource>>> _pConnections; static mutex _connectionsMutex;
	};
	sp<flat_map<string,sp<DB::IDataSource>>> DataSourceApi::_pConnections = make_shared<flat_map<string,sp<DB::IDataSource>>>(); mutex DataSourceApi::_connectionsMutex;
	up<flat_map<string,sp<DataSourceApi>>> _pDataSources = mu<flat_map<string,sp<DataSourceApi>>>(); mutex _dataSourcesMutex;
	sp<DB::Syntax> _pSyntax;
	sp<DB::IDataSource> _pDefault;
	up<vector<function<void()>>> _pDBShutdowns = mu<vector<function<void()>>>();
	α DB::ShutdownClean( function<void()> shutdown )ι->void{
		_pDBShutdowns->push_back( shutdown );
	}
	α DB::CleanDataSources( bool /*terminate*/ )ι->void{
		Trace{ ELogTags::Sql | ELogTags::Shutdown, "CleanDataSources" };
		//DB::ClearQLDataSource(); TODO!
		_pDefault = nullptr;
		Trace{ ELogTags::Sql | ELogTags::Shutdown, "_pDefault=nullptr" };
		for( auto p=_pDBShutdowns->begin(); p!=_pDBShutdowns->end(); p=_pDBShutdowns->erase(p) )
			(*p)();
		_pDBShutdowns = nullptr;
		_pSyntax = nullptr;
		{
			unique_lock l2{DataSourceApi::_connectionsMutex};
			DataSourceApi::_pConnections = nullptr;
		}
		{
			std::unique_lock l{_dataSourcesMutex};
			_pDataSources = nullptr;
		}
		Trace{ ELogTags::Sql | ELogTags::Shutdown, "~CleanDataSources" };
	}
	#define db DataSource()
	DB::Schema _schema;
	α DB::DefaultSchema()ι->Schema&{ return _schema; }
	α DB::CreateSchema()ε->void{
		fs::path path{ Settings::Env("db/meta").value_or((OSApp::Executable().parent_path()/fs::path{OSApp::Executable().filename().stem().string()+"Meta.json"}).string()) };
		if( !fs::exists(path) ){
			path = IApplication::ApplicationDataFolder()/path.filename();
		}
		Information{ ELogTags::Sql, "db meta='{}'", path.string() };
		ordered_json j = Json::Parse( IO::FileUtilities::Load(path) );
		_schema = db.SchemaProc()->CreateSchema( j, path.parent_path() );
	}

	α DB::DefaultSyntax()ι->const DB::Syntax&{
		if( !_pSyntax )
			_pSyntax = Driver()=="Jde.DB.Odbc.dll" ? make_shared<Syntax>() : make_shared<MySqlSyntax>();
		return *_pSyntax;
	}

	α CreateDataSourceLocal( const fs::path& libraryName )ε->sp<DB::IDataSource>{
		static DataSourceApi api{ libraryName };
		return sp<DB::IDataSource>{ api.GetDataSourceFunction(), [](auto) {
					//  delete p;  needs to be deleted before api
		}};
	}

	α DB::CreateDataSource( str connectionString, optional<fs::path> driver )ε->sp<IDataSource>{
		auto p = CreateDataSourceLocal( driver.value_or(Driver()) );
		p->SetConnectionString( connectionString );
		return p;
	}

	α DB::DataSourcePtr()ε->sp<IDataSource>{
		if( !_pDefault ){
			_pDefault = CreateDataSource( Driver() );
			string cs{ Settings::Env("db/connectionString").value_or("DSN=Jde_Log_Connection") };
			_pDefault->SetConnectionString( move(cs) );
		}
		return _pDefault;
	}

	α DB::DataSource()ι->IDataSource&{
		auto p = DataSourcePtr();
		THROW_IF( !p, "No default datasource" );//ie terminate
		return *p;
	}

	std::once_flag _singleShutdown;
	α DB::DataSource( const fs::path& libraryName, string connectionString )->sp<DB::IDataSource>{
		sp<IDataSource> pDataSource;
		std::unique_lock l{_dataSourcesMutex};
		string key = libraryName.string();
		std::call_once( _singleShutdown, [](){ Process::AddShutdownFunction( CleanDataSources ); } );
		auto pSource = _pDataSources->find( key );
		if( pSource==_pDataSources->end() )
			pSource = _pDataSources->emplace( key, make_shared<DataSourceApi>(libraryName) ).first;
		return pSource->second->Emplace( move(connectionString) );
	}

	α DB::Select( string sql, function<void(const IRow&)> f, SL sl )ε->void{ db.Select(move(sql), f, sl); }
	α DB::Select( string sql, function<void(const IRow&)> f, const vector<object>& values, SL sl )ε->void{ db.Select(move(sql), f, values, sl); }

	α DB::IdFromName( sv tableName, string name, SL sl )ι->SelectAwait<uint>{
		return db.ScalerCo<uint>( Jde::format("select id from {} where name=?", tableName), {name}, sl );
	}

	α DB::Execute( string sql, vector<object>&& parameters, SL sl )ε->uint{
		return db.Execute( move(sql), parameters, sl );
	}

	α DB::SelectName( string sql, uint id, sv cacheName, SL sl )ε->CIString{
		CIString y;
		if( auto p = cacheName.size() ? Cache::GetValue<uint,CIString>(string{cacheName}, id) : sp<CIString>{}; p )
			y = CIString{ *p };
		if( y.empty() )
			y = Scaler<CIString>( move(sql), {id}, sl ).value_or( CIString{} );
		return y;
	}

	α DB::SelectIds( string sql, const std::set<uint>& ids, function<void(const IRow&)> f, SL sl )ε->void{
		vector<object> params; params.reserve( ids.size() );
		string s; s.reserve( ids.size()*2 );
		for( var id : ids )
		{
			if( s.size() )
				s+=",";
			s+="?";
			params.emplace_back( id );
		}
		Select( Jde::format("{}({})", move(sql), s), f, params, sl );
	}
}