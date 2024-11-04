#pragma once
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H
#include <jde/framework/coroutine/Await.h>
#include "await/ContainerAwait.h"
#include "await/DBAwait.h"
#include "meta/Column.h"
#include "meta/View.h"
#include "IRow.h"
//#include "meta/DBSchema.h"
#include "generators/Sql.h"


namespace Jde::DB{
	struct IServerMeta; struct Sql; struct Syntax;
	namespace Types{ struct IRow; }

	struct Γ IDataSource : std::enable_shared_from_this<IDataSource>{
		virtual ~IDataSource(){}//warning

		α CatalogName( SRCE )ε->string;
		α SchemaName( SRCE )ε->string;
		β SchemaNameConfig( SRCE )ε->string{ return {}; }
		β AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> = 0; //create new pointing to other catalog.  If have catalogs.
		β AtSchema( sv schema, SRCE )ε->sp<IDataSource> = 0; //create new pointing to other schema.  If can specify schema in connection.
		β ServerMeta()ι->IServerMeta& =0;

		template<class K=uint,class V=string>
		α SelectEnum( const View& table, SRCE )ε->SelectCacheAwait<flat_map<K,V>>{ return SelectMap<K,V>( Ƒ("select {}, name from {}", table.GetPK()->Name, table.DBName), table.Name, sl ); }
		ẗ SelectEnumSync( const View& table, SRCE )ε->sp<flat_map<K,V>>{ return SFuture<flat_map<K,V>>( SelectEnum<K,V>( table, sl) ).get(); }
		ẗ SelectMap( string sql, SRCE )ι->SelectAwait<flat_map<K,V>>;
		ẗ SelectMap( string sql, string cacheName, SRCE )ι->SelectCacheAwait<flat_map<K,V>>;
		ẗ SelectMultiMap( Sql&& statement, SRCE )Ι->up<TAwait<flat_multimap<K,V>>>;
		Ŧ SelectSet( string sql, vector<Value>&& params, SL sl )ι->SelectAwait<flat_set<T>>;
		Ŧ SelectSet( Sql&& sql, SRCE )ι->SelectAwait<flat_set<T>>{ return SelectSet<T>( move(sql.Text), move(sql.Params), sl ); }
		Ŧ SelectSet( string sql, vector<Value>&& params, string cacheName, SL sl )ι->SelectCacheAwait<flat_set<T>>;

		α ScalerNonNull( string sql, vec<Value> params, SRCE )ε->uint;

		Ŧ TryScaler( string sql, vec<Value> params, SRCE )ι->optional<T>;
		Ŧ Scaler( string sql, vec<Value> params, SRCE )ε->optional<T>;
		Ŧ Scaler( Sql&& sql, SRCE )ε->optional<T>{ return Scaler<T>( move(sql.Text), move(sql.Params), sl ); }
		Ŧ ScalerCo( string sql, vec<Value> params, SRCE )ε->SelectAwait<T>;
		Ŧ SelectCo( string sql, vec<Value> params, CoRowΛ<T> fnctn, SRCE )ε->SelectAwait<T>;
		α SelectCo( Sql&& statement, SRCE )Ι->RowAwait{ return RowAwait{ shared_from_this(), move(statement), sl }; }

		α TryExecute( string sql, SRCE )ι->optional<uint>;
		α TryExecute( string sql, vec<Value> params, SRCE )ι->optional<uint>;
		α TryExecuteProc( string sql, vec<Value> params, SRCE )ι->optional<uint>;

		β Execute( string sql, SRCE )ε->uint=0;
		β Execute( string sql, vec<Value> params, SRCE )ε->uint=0;
		β Execute( string sql, const vector<Value>* params, const RowΛ* f, bool isStoredProc=false, SRCE )ε->uint=0;
		β ExecuteCo( string sql, vector<Value> p, SRCE )ι->up<IAwait> =0;
		β ExecuteNoLog( string sql, const vector<Value>* params, RowΛ* f=nullptr, bool isStoredProc=false, SRCE )ε->uint=0;
		β ExecuteProc( string sql, vec<Value> params, SRCE )ε->uint=0;
		β ExecuteProc( string sql, vec<Value> params, RowΛ f, SRCE )ε->uint=0;
		β ExecuteProcCo( string sql, vector<Value> params, SRCE )ι->up<IAwait> =0;
		β ExecuteProcCo( string sql, vector<Value> params, RowΛ f, SRCE )ε->up<IAwait> =0;
		β ExecuteProcNoLog( string sql, vec<Value> params, SRCE )ε->uint=0;

		α Select( string sql, RowΛ f, vec<Value> params, SRCE )ε->void;
		α Select( string sql, RowΛ f, SRCE )ε->void;
		β Select( string sql, RowΛ f, const vector<Value>* pValues, SRCE )ε->uint=0;
		β Select( Sql&& s, SRCE )Ε->vector<up<IRow>> =0;
		β SelectNoLog( string sql, RowΛ f, const vector<Value>* pValues, SRCE )ε->uint=0;
		α TrySelect( string sql, RowΛ f, SRCE )ι->bool;

		α CS()const ι->str{ return _connectionString; }
		β SetConnectionString( string x )ι->void{ _connectionString = move(x); _schema.clear(); _catalog.reset(); }
		//α Schema()ι->const Schema&{ return _schema; }
		β Syntax()ι->const Syntax& = 0;

	protected:
		string _connectionString;
		//DB::Schema _schema;  idea is to keep separate since db/schema can have multiple configured schemas.
		optional<string> _catalog; //db catalog name ie dbo
		string _schema;  //db schema name ie dbo
	private:
		β SelectCo( ISelect* pAwait, string sql, vector<Value>&& params, SRCE )ι->up<IAwait> =0;
		friend struct ISelect;
	};
#define let const auto
	Ŧ IDataSource::TryScaler( string sql, vec<Value> params, SL sl )ι->optional<T>{
		try{
			return Scaler<T>( move(sql), params, sl );
		}
		catch( IException& ){
			return nullopt;
		}
	}
	Ŧ IDataSource::Scaler( string sql, vec<Value> params, SL sl )ε->optional<T>{
		optional<T> result;
		Select( move(sql), [&result](const IRow& row){ result = row.Get<T>(0); }, params, sl );
		return result;
	}

	Ŧ IDataSource::ScalerCo( string sql, vec<Value> params, SL sl )ε->SelectAwait<T>{
		auto f = []( T& y, const IRow& r ){ y = r.Get<T>( 0 ); };
		return SelectAwait<T>( shared_from_this(), move(sql), f, params, sl );
	}
	Ŧ IDataSource::SelectCo( string sql, vec<Value> params, CoRowΛ<T> f, SL sl )ε->SelectAwait<T>{
		//auto f = []( T& y, const IRow& r ){ y = r.Get<T>( 0 ); };
		return SelectAwait<T>( shared_from_this(), move(sql), f, params, sl );
	}

	namespace zInternal{
		ẗ ProcessMapRow( flat_map<K,V>& y, const IRow& row )ε{ y.emplace( row.Get<K>(0), row.Get<V>(1) ); }
		Ŧ ProcessSetRow( flat_set<T>& y, const IRow& row )ε{ y.emplace( row.Get<T>(0) ); }
	}

	ẗ IDataSource::SelectMap( string sql, SL sl )ι->SelectAwait<flat_map<K,V>>{
		return SelectAwait<flat_map<K,V>>( shared_from_this(), move(sql), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ẗ IDataSource::SelectMap( string sql, string cacheName, SL sl )ι->SelectCacheAwait<flat_map<K,V>>{
		return SelectCacheAwait<flat_map<K,V>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessMapRow<K,V>, {}, sl );
	}
	ẗ IDataSource::SelectMultiMap( Sql&& s, SL sl )Ι->up<TAwait<flat_multimap<K,V>>>{
		MultimapAwait<K,V> m{ shared_from_this(), move(s), sl };
		return mu<MultimapAwait<K,V>>( move(m) );
		//return mu<MultimapAwait<K,V>>( shared_from_this(), move(s), sl );
	}

	Ŧ IDataSource::SelectSet( string sql, vector<Value>&& params, SL sl )ι->SelectAwait<flat_set<T>>{
		return SelectAwait<flat_set<T>>( shared_from_this(), move(sql), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	Ŧ IDataSource::SelectSet( string sql, vector<Value>&& params, string cacheName, SL sl )ι->SelectCacheAwait<flat_set<T>>{
		return SelectCacheAwait<flat_set<T>>( shared_from_this(), move(sql), move(cacheName), zInternal::ProcessSetRow<T>, move(params), sl );
	}
	Ξ ISelect::SelectCo( string sql, vector<Value>&& params, SL sl )ι->up<IAwait>{
		return _ds->SelectCo( this, move(sql), move(params), sl );
	}
}
#undef let
#endif