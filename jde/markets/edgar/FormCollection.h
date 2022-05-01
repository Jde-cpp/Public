#pragma once
#include "../../../../XZ/source/XZ.h"
#include <jde/markets/types/proto/edgar.pb.h>
#include "types/Forms.h"
#include "types/Filing.h"

#define Φ ΓE α
namespace Jde::Markets::Edgar
{
	struct ΓE IFormCollection : std::enable_shared_from_this<IFormCollection>
	{
		Ω Create( EForm form )ε->sp<IFormCollection>;
		Ω Alias( EForm f )ι->EForm{ return /*f==EForm::_13GA ? EForm::_13G :*/ f; }
		β Load( Period filingPeriod, sp<Proto::MasterIndex> pIndex={} )ι->AsyncReadyAwait=0;
		α Path( Period filingPeriod )Ι->fs::path;
		α Path( const Proto::Filing& filing )Ι->fs::path;
		β Process( Proto::Filing filing, sp<Proto::MasterIndex> pIndex )ι->AsyncAwait=0;
		α HaveForm( Proto::Filing filing/*, const Period submitPeriod, SRCE*/ )ι->bool{ return filing.period_end(); }
		β Read( google::protobuf::Message& m, const Xml::XMLElement& root )ε->void=0;
		α UpdateIndex( Proto::MasterIndex& index, const Form& form )ι->void;
		α VerifySaved( Proto::MasterIndex& index, EForm eType, Period filingPeriod, function<bool(Cik,uint32_t)> hasLine )ι->void;
		β Save( const Period& period )ε->void=0;
		β Loaded( const Period& period )ι->bool=0;
		β Emplace( Period period, const sp<Form>& p )ι->void=0;//{ _forms.try_emplace( period ).first->second->emplace( cik, move(p) ); }
		β GetPeriod( const Xml::XMLElement& root )ε->DayIndex=0;
		Ω Fetch( Proto::Filing filing )ι->AsyncAwait;
		const EForm Type;
	protected:
		IFormCollection( EForm form )noexcept:Type{form}{}
		static constexpr sv Url = "www.sec.gov"sv;
		Ω Target( const Proto::Filing& filing )ι{ return format( "/Archives/edgar/data/{}/{}", filing.cik(), Filing::FileName(filing) ); }
		Ṫ CreateForm( const Proto::Filing& filing, const Period& submitPeriod )ι->up<T>;
	};

	ⓣ IFormCollection::CreateForm( const Proto::Filing& filing, const Period& submitPeriod )ι->up<T>
	{
		auto p = mu<T>(); auto& l = *p->add_lines();
		l.set_quarter( submitPeriod.Quarter ); l.set_year( submitPeriod.Year ); l.set_line_number( filing.line_number() ); //l.set_cik( filing.cik() );
		return p;
	}

	struct Form13G : Form
	{
		Form13G( Proto::Form13G f, Period periodEnd )noexcept:_periodEnd{periodEnd},_proto{move(f)}{}
		α Lines()Ι->const google::protobuf::RepeatedPtrField<Proto::IndexLine>& override{ return _proto.lines(); }
		α Type()Ι->EForm{ return EForm::_13G; }
		α Cik()Ι->Markets::Cik override{ return _proto.cik(); }
		β PeriodEnd()Ι->Period override{ return _periodEnd; };
		α Items()Ι->const google::protobuf::RepeatedPtrField<Proto::Form13GItem>&{ return _proto.items(); }
		operator const Proto::Form13G&()Ι{return _proto;}
	private:
		Period _periodEnd;
		Proto::Form13G _proto;
	};
	struct ΓE FormCollection13G final: IFormCollection
	{
		FormCollection13G()ι:IFormCollection{EForm::_13G}{}
		α Load( Period filingPeriod, sp<Proto::MasterIndex> pIndex={} )ι->AsyncReadyAwait override;
		α Loaded( const Period& period )ι->bool override{ return _data.find(period)!=_data.end(); }
		α Save( const Period& period )ε->void override;
		α Emplace( Period period, const sp<Form>& pForm )ι->void override;
		α Process( Proto::Filing filing, sp<Proto::MasterIndex> pIndex )ι->AsyncAwait override;
		α Read( google::protobuf::Message& m, const Xml::XMLElement& root )ε->void override;
		α GetPeriod( const Xml::XMLElement& root )ε->DayIndex override;
	private:
		flat_map<Period,flat_map<Cik,flat_map<string, sp<Form13G>>>> _data;	//nk period, underlying cik & investor name.
	};
}
#undef Φ