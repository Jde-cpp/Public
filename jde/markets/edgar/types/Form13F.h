#pragma once
#include "../FormCollection.h"
#include "Forms.h"
#include "../EdgarDefs.h"
#include "../../../../../XZ/source/XZ.h"
#include <jde/markets/types/proto/edgar.pb.h>

#define Φ ΓE α
namespace Jde::Markets::Edgar
{
	struct IFormCollection;
	struct Form13F final : Form
	{
		Form13F( Proto::Form13F&& x, Period periodEnd ):_periodEnd{periodEnd},_proto{ move(x) }{}

		α Lines()Ι->const google::protobuf::RepeatedPtrField<Proto::IndexLine>&  override{ return _proto.lines(); }
		α Cik()Ι->Markets::Cik override{ return _proto.cik(); }
		α PeriodEnd()Ι->Period override{ return _periodEnd; };
		α Type()Ι->EForm override{ return EForm::_13F; }
		α Proto()Ι->const Proto::Form13F&{ return _proto; }
		α LatestLine()Ι->const Proto::IndexLine*;
		α Holdings()Ι->const google::protobuf::RepeatedPtrField<Proto::InfoTable>&{ return _proto.holdings(); }
		α SetHoldings( const google::protobuf::RepeatedPtrField<Proto::InfoTable>& x )ι->void;
	private:
		Period _periodEnd;
		Proto::Form13F _proto;
	};

	struct ΓE FormCollection13F final: IFormCollection
	{
		FormCollection13F()ι:IFormCollection{EForm::_13F}{}
//		~FormCollection13G(){}
		//α Find( const Proto::Filing& filing )ι->Form* override;
		α Load( Period filingPeriod, sp<Proto::MasterIndex> pIndex={} )ι->AsyncReadyAwait override;
		α Loaded( const Period& period )ι->bool override{ return _data.find(period)!=_data.end(); }
		α Save( const Period& period )ε->void override;
		α Emplace( Period period, const sp<Form>& pForm )ι->void override;
		α Process( Proto::Filing filing, sp<Proto::MasterIndex> pIndex )ι->AsyncAwait override;
		α Read( google::protobuf::Message& m, const Xml::XMLElement& root )ε->void override;
		α GetPeriod( const Xml::XMLElement& root )ε->DayIndex override;
	private:
		flat_map<Period,flat_map<Cik,sp<Form13F>>> _data;	//nk period, cik & investments.
	};

	struct Form13FOld : Form
	{
		Form13FOld( up<Proto::Form13FOld>&& x ):_data{ move(x) }
		{
			/*_line.set_cik( _data->cik() );
			_line.set_quarter( _data->quarter() );
			_line.set_year( _data->year() );
			_line.set_line_number( _data->line_number() );
			*/

		}
		α IndexLine()Ι->const Proto::IndexLine&{ return _line; }
	private:
		up<Proto::Form13FOld> _data;
		Proto::IndexLine _line;
	};
}

namespace Jde::Markets::Edgar
{
	using ContractPK = uint32;
	Φ Process( sp<Proto::MasterIndex> pIndex, sp<IFormCollection> pForms )noexcept(false)->void;
	α Consolidate( path current, path consolidated )noexcept(false)->void;

	Φ LoadInvestors( sv name, Period period )noexcept(false)->up<Proto::Investors>;
	Φ LoadInvestors( Cik cik, Period period )noexcept(false)->up<Proto::Investors>;
	Ξ LoadInvestors( sv name )noexcept(false)->up<Proto::Investors>{ return LoadInvestors(name, Period::Current()-1); }
	Ξ LoadInvestors( Cik cik )noexcept(false)->up<Proto::Investors>{ return LoadInvestors( cik, Period::Current()-1); }
}
#undef Φ