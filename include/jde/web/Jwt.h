#pragma once
#include <jde/crypto/OpenSsl.h>
#include "client/exports.h"

namespace Jde::Web{
	struct ΓWC Jwt{
		Jwt()ι{ ASSERT(false); }
		Jwt( sv jwt )ε;
		Jwt( Crypto::PublicKey publicKey, Jde::UserPK userPK, str userName, str userTarget, SessionPK sessionId, str endpoint, TimePoint expires, str description, const fs::path& privateKeyPath )ι;
		Jwt( Crypto::PublicKey publicKey, Jde::UserPK userPK, str userName, str userTarget )ι: PublicKey{move(publicKey)}, UserPK{userPK}, UserName{userName}, UserTarget{userTarget}{}
		α Payload()Ι->string;
		α Aud()Ε->string{ return Json::AsString( Body, "aud" ); }
		α Iss()Ι->sv{ return Json::FindDefaultSV( Body, "iss" ); }
		α Expires()Ι->TimePoint{ return Clock::from_time_t(Iat); }
		string Kid;
		jobject Body;
		string HeaderBodyEncoded;
		Crypto::Signature Signature;
		Crypto::PublicKey PublicKey;
		string Host;
		time_t Iat;
		string SessionId;
		Jde::UserPK UserPK;
		string UserName;
		string UserTarget;
		string Description;
		α SetModulus( str encoded )ι->void;
		α SetExponent( str encoded )ι->void;
	};
}