#pragma once
#include <jde/ql/QLHook.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/fwk/co/Await.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/meta/Table.h>

namespace Jde::DB{ struct Sql; }
namespace Jde::QL{
	struct PurgeAwait final: TAwait<jvalue>{
		using base=TAwait<jvalue>;
		PurgeAwait( sp<DB::Table> table, MutationQL mutation, UserPK userPK, SRCE )ι;
		α Suspend()ι->void override{ Before(); }
	private:
		α Before()ι->MutationAwaits::Task;
		α Statements( const DB::Table& table )ε->vector<DB::Sql>;
		α Execute()ι->DB::ExecuteAwait::Task;
		α After( up<IException>&& e )ι->MutationAwaits::Task;
		α After( uint y )ι->MutationAwaits::Task;
		α Resume( jvalue&& v )ι->void;
		const MutationQL _mutation;
		sp<DB::Table> _table;
		UserPK _userPK;
		jobject _variables;
	};
}