#pragma once
#include <jde/framework/process.h>
#include <jde/markets/Exports.h>
#include <jde/markets/types/proto/edgar.pb.h>
#include "../../../../Framework/source/Settings.h"
#include "types/Forms.h"

#define Φ ΓE auto
namespace Jde::Markets::Edgar
{
	α operator <=( const Edgar::Proto::IndexLine& l1, const Edgar::Proto::IndexLine& l2 )ι->bool;
	Ξ operator >( const Edgar::Proto::IndexLine& l1, const Edgar::Proto::IndexLine& l2 )ι->bool{ return !(l1<=l2); }
	namespace Proto{ class MasterIndex; }
	struct IFormCollection;
	Φ LoadFilings( Cik cik )ε->up<Edgar::Proto::Filings>;
	Φ Process( Period submitPeriod )ε->AsyncAwait;
	Φ Process( sp<Proto::MasterIndex> index_, flat_set<EForm> eForms, SRCE )ε->AsyncAwait;
	α CompressionAmount()noexcept->uint8;
	Ξ EdgarPath()noexcept->fs::path{ auto x = Settings::TryGetSubcontainer<fs::path>( "edgar"sv, "dir"sv ); return x.value_or(IApplication::ApplicationDataFolder()/"edgar"); }
	Ξ PeriodPath( const Period& period )noexcept->fs::path{ return EdgarPath()/period.Dir(); }
	Ξ IndexPath( const Period& submitPeriod )noexcept->fs::path{ return PeriodPath(submitPeriod)/"index.dat"; }
}
#undef Φ