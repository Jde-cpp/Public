#pragma once
#include <jde/framework/coroutine/Await.h>

namespace Jde::Access{
	struct LoginAwait final : TAwait<UserPK>{
		using base = TAwait<UserPK>;
		LoginAwait( vector<unsigned char> modulus, vector<unsigned char> exponent, string&& name, string&& target, string&& description, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α LoginTask()ε->Jde::Task;
		vector<unsigned char> _modulus; vector<unsigned char> _exponent; string _name; string _target; string _description;
	};
}