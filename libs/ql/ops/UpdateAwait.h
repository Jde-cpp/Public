#pragma once
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/QLHook.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/generators/UpdateClause.h>
#include <jde/framework/co/Await.h>

namespace Jde::DB{ struct Table; struct IDataSource; }
namespace Jde::QL{
	struct UpdateAwait final: TAwait<jvalue>{
		using base=TAwait<jvalue>;
		UpdateAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι;
		α await_ready()ι->bool override;
		α Suspend()ι->void override{ UpdateBefore(); }
		α await_resume()ε->jvalue override;
	private:
		α CreateUpdate( const DB::Table& table )ε->DB::Value;
		α CreateDeleteRestore( const DB::Table& table )ε->void;
		α UpdateAfter( uint rowCount )ι->MutationAwaits::Task;
		α UpdateBefore()ι->MutationAwaits::Task;
		α AddStatement( const DB::Table& table, const jobject& input, bool nested, str criteria={} )ε->void;
		α Execute()ι->DB::ExecuteAwait::Task;
		α Resume( jvalue&& v )ι->void;

		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		up<IException> _exception;
		vector<DB::UpdateClause> _updates;
	};
}