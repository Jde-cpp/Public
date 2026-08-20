#pragma once
#include "../Row.h"
#include "SelectAwait.h"
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	ΓDB α ScalerAwaitExecute( IDataSource& _ds, variant<Sql,InsertClause>&& _sql, function<void(optional<Row>)> onRow, function<void(Exception&&)> onError, SL sl )ι->QueryAwait::Task;

	Τ struct ScalerAwaitOpt : TAwaitEx<optional<T>,void>{
		using base=TAwaitEx<optional<T>,void>;
		ScalerAwaitOpt( sp<IDataSource> ds, variant<Sql,InsertClause>&& s, SL sl )ι: base{ sl }, _ds{ move(ds) }, _sql{ move(s) }{}
		α Execute()ι->void override;
		α OnRow( optional<Row> result )ι->void;
		α OnError( Exception&& e )ι->void;
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};

	Τ α ScalerExecute( TAwait<T>& self, sp<IDataSource>&& ds, variant<Sql,InsertClause>&& sql, SL sl )ι->ScalerAwaitOpt<T>::Task{
		try{
			auto opt = co_await ScalerAwaitOpt<T>{ move(ds), move(sql), sl };
			if( opt )
				self.Resume( move(*opt) );
			else
				self.ResumeExp( Exception{"No value returned", ELogLevel::Error, sl} );
		}
		catch( runtime_error& e ){
			self.ResumeExp( move(e) );
		}
	}

	Τ struct ScalerAwait : TAwait<T>{
		using base = TAwait<T>;
		ScalerAwait( sp<IDataSource> ds, variant<Sql,InsertClause>&& s, SL sl )ι:base{sl}, _ds{ds}, _sql{move(s)}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->ScalerAwaitOpt<T>::Task{ return ScalerExecute<T>( *this, move(_ds), move(_sql), base::_sl ); }
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};

	//explicit specialization so IAwait<uint32,...> is exported once from here instead of separately by every dll that does ScalerAwait<uint32> (e.g. DS().InsertSeq<uint32>/Scaler<uint32>).
	template<> struct ΓDB ScalerAwait<uint32> : UInt32Await{
		using base = UInt32Await;
		ScalerAwait( sp<IDataSource> ds, variant<Sql,InsertClause>&& s, SL sl )ι:base{sl}, _ds{ds}, _sql{move(s)}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->ScalerAwaitOpt<uint32>::Task;
	private:
		sp<IDataSource> _ds;
		variant<Sql,InsertClause> _sql;
	};

	Ŧ ScalerAwaitOpt<T>::Execute()ι->void{
		if( std::holds_alternative<InsertClause>(_sql) )
			get<InsertClause>(_sql).Add( T{} );
		ScalerAwaitExecute( *_ds, move(_sql),
		[this](optional<Row> r){OnRow(move(r));},
		[this](Exception&& e){OnError(move(e));},
		base::_sl );
	}
	//#53: GetOpt, not Get.  Get on a NULL cell ASSERTs and returns T{}, so the async ScalerOpt answered an *engaged*
	//optional{0} where ScalerSyncOpt - which goes through Row::GetOpt - answers nullopt, and Scaler<T> then returned 0
	//instead of reporting "No value returned".  One API, one answer.  The try/catch is gone with it: Row::Get and
	//ResumeScaler are both Ι, so nothing in here could ever have thrown.
	Ŧ ScalerAwaitOpt<T>::OnRow( optional<Row> r )ι->void{
		base::ResumeScaler( r ? r->template GetOpt<T>(0) : optional<T>{} );
	}
	Ŧ ScalerAwaitOpt<T>::OnError( Exception&& e )ι->void{
		base::ResumeExp( move(e) );
	}
}