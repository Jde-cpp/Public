#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/awaits/ScalerAwait.h>

namespace Jde::Access::Server{
	struct LoginAwait final : TAwait<UserPK>{
		using base = TAwait<UserPK>;
		LoginAwait( vector<unsigned char> modulus, vector<unsigned char> exponent, string&& name, string&& target, string&& description, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α LoginTask()ι->TAwait<optional<UserPK::Type>>::Task;
		α InsertUser( string&& modulusHex, uint32_t exponent )ι->DB::ScalerAwait<UserPK::Type>::Task;
		vector<unsigned char> _modulus; vector<unsigned char> _exponent; string _name; string _target; string _description;
	};
}