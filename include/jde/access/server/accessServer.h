#pragma once
#include <span>
#include <jde/fwk/co/Await.h>
#include <jde/ql/IQLSession.h>
#include "awaits/AuthenticateAwait.h"
#include "../awaits/ConfigureAwait.h"

namespace Jde::Access{ struct Authorize; }
namespace Jde::Crypto{ struct TrustStore; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{ struct LocalQL; struct TableQL; struct MutationQL; }

namespace Jde::Access::Server{
	α Authenticate( str loginName, uint providerId, str opcServer={}, SRCE )ι->AuthenticateAwait;
	α Trust()ε->Crypto::TrustStore&;//anchor store for key-login enrollment: /access/trustedCertDirs + AddCertificate'd anchors - never OS roots, a public-CA cert must not authorize enrollment.
	α TrustVerify( std::span<const byte> der, SRCE )ε->void;//Trust().Verify, rescanning /access/trustedCertDirs on failure - an anchor copied in after startup works without a restart.
	α Configure( vector<sp<DB::AppSchema>>&& schemas, sp<QL::LocalQL> localQL, UserPK executer, sp<Authorize> authorizer, sp<AccessListener> listener )ε->ConfigureAwait;
	α CustomQuery( QL::TableQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>;
	α CustomMutation( QL::MutationQL& ql, QL::Creds executer, SL sl )ι->up<TAwait<jvalue>>;
}