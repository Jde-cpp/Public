#pragma once
#include <jde/ql/types/MutationQL.h>

namespace Jde::DB{ struct AppSchema; }
namespace Jde::App::Server{
	struct InstanceTagLevelAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		InstanceTagLevelAwait( QL::TableQL&& q, SRCE )ι:base{sl}, _query{move(q)}{}
		Ω IsApplicable( const QL::TableQL& q )ι->bool{ return q.JsonName.starts_with("instanceTagLevel"); }
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<vector<DB::Row>>::Task;
	private:
		QL::TableQL _query;
	};

	struct InstanceTagLevelMAwait final : TAwait<jvalue>{
		using base = TAwait<jvalue>;
		InstanceTagLevelMAwait( QL::MutationQL&& m, UserPK executer, SRCE )ι:base{sl}, _mutation{move(m)}, _executer{executer}{}
		Ω IsApplicable( const QL::MutationQL& m )ι->bool{ return m.CommandName=="updateInstanceTagLevel"; }
		α Suspend()ι->void override{ Update(); }
		α Update()ι->TAwait<uint32>::Task;
	private:
		QL::MutationQL _mutation;
		UserPK _executer;
	};
}
