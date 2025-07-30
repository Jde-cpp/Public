#pragma once

namespace Jde::Opc::Server{
	struct UpsertAwait final : VoidAwait{
		α await_ready()ι->bool override;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()->TAwait<jvalue>::Task;

		vector<fs::path> _files;
	};
	α Upsert( string query, UserPK executer )ε->jarray;
}