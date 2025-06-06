#pragma once
#include "../IRow.h"
#include "SelectAwait.h"
#include <jde/db/generators/Statement.h>

#define let const auto

namespace Jde::DB{
	ΓDB α ScalerAwaitExecute( sp<IDataSource>&& _ds, Sql&& _sql, function<void(variant<optional<Row>,up<IException>>&&)> callback, SL sl )ι->SelectAwait::Task;

	Τ struct ScalerAwaitOpt : TAwaitEx<optional<T>,void>{
		using base=TAwaitEx<optional<T>,void>;
		ScalerAwaitOpt( sp<IDataSource> ds, Sql&& s, SL sl )ι: base{ sl }, _ds{ move(ds) }, _sql{ move(s) }{}
		α Execute()ι->void override;
		α Callback( variant<optional<Row>,up<IException>>&& )ι->void;
	private:
		sp<IDataSource> _ds;
		Sql _sql;
	};

	Τ struct ScalerAwait : TAwait<T>{
		using base = TAwait<T>;
		ScalerAwait( sp<IDataSource> ds, Sql&& s, SL sl )ι:base{sl}, _ds{ds}, _sql{move(s)}{}
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->ScalerAwaitOpt<T>::Task{
			try{
				auto opt = co_await ScalerAwaitOpt<T>{ move(_ds), move(_sql), base::_sl };
				if( opt )
					base::Resume( move(*opt) );
				else
					base::ResumeExp( Exception{"No value returned", ELogLevel::Error, base::_sl} );
			}
			catch( IException& e ){
				base::ResumeExp( move(e) );
			}
		}
	private:
		sp<IDataSource> _ds;
		Sql _sql;
	};


	Ŧ ScalerAwaitOpt<T>::Execute()ι->void{
		ScalerAwaitExecute( move(_ds), move(_sql), [this](variant<optional<Row>,up<IException>>&& result){Callback(move(result)); }, base::_sl );
	}
	Ŧ ScalerAwaitOpt<T>::Callback( variant<optional<Row>,up<IException>>&& result )ι->void{
		if( result.index() == 1 )
			base::ResumeExp( move(*get<1>(move(result))) );
		else{
			auto row = get<0>(move(result));
			try{
				base::ResumeScaler( row ? row->template Get<T>(0) : optional<T>{} );
			}
			catch( IException& e ){
				base::ResumeExp( move(e) );
			}
		}
	}
}
