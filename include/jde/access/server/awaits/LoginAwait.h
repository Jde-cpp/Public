#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include <jde/db/awaits/ScalerAwait.h>

namespace Jde::Access::Server{
	struct LoginAwait final : TAwait<UserPK>{
		using base = TAwait<UserPK>;
		LoginAwait( Crypto::PublicKey certificate, string&& name, string&& target, string&& description, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α LoginTask()ι->TAwait<optional<UserPK::Type>>::Task;
		α InsertUser( string&& modulusHex, uint32_t exponent )ι->DB::ScalerAwait<UserPK::Type>::Task;
		Crypto::PublicKey _publicKey; string _name; string _target; string _description;
	};
}