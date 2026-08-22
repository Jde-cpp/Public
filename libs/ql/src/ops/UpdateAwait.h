#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/QLHook.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/generators/UpdateClause.h>
#include <jde/fwk/co/Await.h>

namespace Jde::DB{ struct Table; struct View; struct IDataSource; }
namespace Jde::QL{
	struct UpdateAwait final: TAwait<jvalue>{
		using base=TAwait<jvalue>;
		UpdateAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ Build(); }
		α await_resume()ε->jvalue override;
	private:
		α CreateUpdate( const DB::Table& table )ε->DB::Value;
		α CreateDeleteRestore( const DB::Table& table )ε->void;
		α Build()ι->TTask<flat_map<uint,string>>; //=CacheAwait<flat_map<uint,string>>::Task - an awaitable dictates its caller's return type, so the enum load is its own step in the chain.
		α FlagTables( const DB::Table& table, vector<sp<DB::View>>& y )ι->void;
		α EnumValues( const DB::View& enumTable )ε->const flat_map<uint,string>&;
		α UpdateAfter( uint rowCount )ι->MutationAwaits::Task;
		α UpdateBefore()ι->MutationAwaits::Task;
		α AddStatement( const DB::Table& table, const jobject& input, bool nested, str criteria={} )ε->void;
		α Execute()ι->DB::ExecuteAwait::Task;
		α Resume( jvalue&& v )ι->void;

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		up<Exception> _exception;
		vector<DB::UpdateClause> _updates;
		jobject _input;
		flat_map<string,flat_map<uint,string>> _enums; //enum table name -> its values, fetched by UpdateBefore so the synchronous build never waits on the db.
	};
}