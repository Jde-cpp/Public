#pragma once
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H
#include <jde/fwk/co/Await.h>
#include "awaits/MapAwait.h"
#include "awaits/DBAwait.h"
#include "awaits/ExecuteAwait.h"
#include "awaits/OutAwait.h"
#include "awaits/QueryAwait.h"
#include "awaits/ScalerAwait.h"
#include "awaits/SelectAwait.h"
#include "meta/Column.h"
#include "meta/View.h"
#include "Row.h"
#include "generators/InsertClause.h"

namespace Jde::DB{
	struct IServerMeta; struct Sql; struct Syntax;

	struct ΓDB IDataSource : std::enable_shared_from_this<IDataSource>{
		virtual ~IDataSource(){}//warning
		β Disconnect()ε->void = 0;
		α CatalogName( SRCE )ε->string;
		α SchemaName( SRCE )ε->string;
		β SchemaNameConfig( SL=SRCE_CUR )ι->string{ return {}; } //schema name in connection string.
		β SetConfig( const jobject& config )ε->void=0;
		β AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> = 0; //create new pointing to other catalog.  If have catalogs.
		β AtSchema( sv schema, SRCE )ε->sp<IDataSource> = 0; //create new pointing to other schema.  If can specify schema in connection.
		β ServerMeta()ι->IServerMeta& =0;

		Ŧ ScalerSync( Sql&& sql, SRCE )ε->T;
		Ŧ ScalerSyncOpt( Sql&& sql, SRCE )ε->optional<T>;
		Ŧ Scaler( Sql&& sql, SRCE )ι{ return ScalerAwait<T>{ shared_from_this(), move(sql), sl }; }
		Ŧ ScalerOpt( Sql&& sql, SRCE )ι{ return ScalerAwaitOpt<T>{ shared_from_this(), move(sql), sl }; }

		β Select( Sql&& s, SRCE )ε->vector<Row> =0;
		β Select( Sql&& s, RowΛ f, SRCE )ε->uint =0;
		α SelectAsync( Sql&& sql, SRCE )ι->SelectAwait{ return SelectAwait{ shared_from_this(), move(sql), sl }; }
		template<class K=uint,class V=string>
		α SelectEnum( const View& table, SRCE )ε->CacheAwait<flat_map<K,V>>{ return SelectMap<K,V>( {Ƒ("select {}, name from {}", table.GetPK()->Name, table.DBName)}, table.Name, sl ); }
		ẗ SelectEnumSync( const View& table, SRCE )ε->flat_map<K,V>{
			return BlockAwait<CacheAwait<flat_map<K,V>>,flat_map<K,V>>( SelectEnum<K,V>( table, sl) );
		}
		α TrySelect( Sql&& s, RowΛ f, SRCE )ι->bool;

		ẗ SelectMap( Sql&& sql, string cacheName, SRCE )ι->CacheAwait<flat_map<K,V>>;
		ẗ SelectMap( Sql&& sql, SRCE )ι->MapAwait<K,V>{ return MapAwait<K,V>{shared_from_this(), move(sql), sl}; }
		ẗ SelectMultiMap( Sql&& sql, SRCE )ι->up<TAwait<flat_multimap<K,V>>>;
		Ŧ SelectSet( Sql&& sql, SRCE )ι->TSelectAwait<flat_set<T>>{ return SelectSet<T>( move(sql.Text), move(sql.Params), sl ); }
		Ŧ SelectSet( Sql&& sql, string cacheName, SRCE )ι->CacheAwait<flat_set<T>>;

		α TryExecuteSync( Sql&& sql, SRCE )ι->optional<uint>;

		[[nodiscard]] α Execute( Sql&& sql, SRCE )ε->ExecuteAwait{ return ExecuteAwait{shared_from_this(), move(sql), sl}; }
		Ŧ ExecuteScaler( Sql&& sql, EValue outValue, SRCE )ε->OutAwait<T>{ return OutAwait<T>{shared_from_this(), move(sql), outValue, sl}; }
		β ExecuteSync( Sql&& sql, SRCE )ε->uint=0;
		β ExecuteScalerSync( Sql&& sql, EValue outValue, SRCE )ε->DB::Value=0;
		Ŧ InsertSeq( DB::InsertClause&& sql, SRCE )ι{ return ScalerAwait<T>{ shared_from_this(), move(sql), sl }; }
		Ŧ InsertSeqSync( DB::InsertClause&& insert, SRCE )ε->T{ return static_cast<T>( InsertSeqSyncUInt(move(insert), sl) ); }
		β ExecuteNoLog( Sql&& sql, SRCE )ε->uint=0;
		β Query( Sql&& sql, bool outParams=false, SRCE )ε->QueryAwait=0;
		β Syntax()ι->const Syntax& = 0;

	protected:
		β InsertSeqSyncUInt( DB::InsertClause&& insert, SL sl )ε->uint=0;
		optional<string> _catalog; //db catalog name ie jde
		string _schema;  //db schema name ie dbo
	private:
		friend struct ISelect;
	};
#define let const auto
	Ŧ IDataSource::ScalerSyncOpt( Sql&& sql, SL sl )ε->optional<T>{
		optional<T> y;
		Select( move(sql), [&y](Row&& row){ y = row.GetOpt<T>(0); }, sl );
		return y;
	}
	Ŧ IDataSource::ScalerSync( Sql&& sql, SL sl )ε->T{
		auto y = ScalerSyncOpt<T>( move(sql), sl );
		THROW_IFSL( !y, "No value returned from scaler query." );
		return *y;
	}

	namespace zInternal{
		ẗ ProcessMapRow( flat_map<K,V>& y, Row&& row )ε{ y.emplace( row.Get<K>(0), row.Get<V>(1) ); }
		Ŧ ProcessSetRow( flat_set<T>& y, Row&& row )ε{ y.emplace( row.Get<T>(0) ); }
	}

	ẗ IDataSource::SelectMap( Sql&& sql, string cacheName, SL sl )ι->CacheAwait<flat_map<K,V>>{
		return CacheAwait<flat_map<K,V>>( shared_from_this(), move(sql), zInternal::ProcessMapRow<K,V>, move(cacheName), sl );
	}

	ẗ IDataSource::SelectMultiMap( Sql&& s, SL sl )ι->up<TAwait<flat_multimap<K,V>>>{
		MultimapAwait<K,V> m{ shared_from_this(), move(s), sl };
		return mu<MultimapAwait<K,V>>( move(m) );
	}
}
#undef let
#endif