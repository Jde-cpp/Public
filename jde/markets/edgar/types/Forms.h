#pragma once
#include "../../../../../Framework/source/DateTime.h"
#include "../../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/io/tinyxml2.h>
#include "../Exports.h"
#include "../EdgarDefs.h"

#define Φ ΓE auto
namespace Jde::Markets::Edgar
{
	enum class EForm : uint32/*sync with db meta*/
	{
		None = 0,
		_10K = 1,
		_2E = 2,
		_3 = 3,
		_4 = 4,
		_5 = 5,
		_6K = 6,
		_7M = 7,
		_8K = 8,
		_9M = 9,
		_10Q = 10,
		_11K = 11,
		_12B25 = 12,
		_13F = 13,
		_13GA = 137,
		_13G = 138,
		_114 = 114
	};
	α ToString( EForm form )ι->sv;
	α ToForm( sv name )ι->EForm;
	Φ ProcessForms()ι->flat_set<EForm>;

	struct ΓE Period
	{
		Period( uint16 year, uint8 quarter )ι:Year{year},Quarter{quarter}{};
		Period( DayIndex day )ι;
		operator bool()Ι{return Year!=0 && Quarter!=0;}
		friend α operator>( const Period& a, const Period& b )ι->bool{ return !(a<b); }
		friend α operator<( const Period& a, const Period& b )ι->bool{ return a.Year==b.Year ? a.Quarter<b.Quarter : a.Year<b.Year; }

		Ω Current()ι{ return Period{Chrono::ToDays(Clock::now())}; }
		α operator--()ι->Period&{ --Quarter; if( Quarter==0 ){Quarter=4; --Year;} return *this; }
		α operator-(int x)Ι{ auto p=*this; for(uint i=0; i<std::abs(x); ++i) --p; return p; }//doesn't work with Period- -5
		α End()Ι->TimePoint;
		operator DayIndex()Ι{ return Chrono::ToDays( End() ); }
		α Dir()Ι->fs::path{ return format("{:04d}-{}", Year, Quarter); }
		α ToString()Ι->string{ return format("{}-{:02d}", Quarter, Year-2000); }

		uint16 Year{0};
		uint8 Quarter{0};
	};

	struct Form
	{
		virtual ~Form()=0;
		Ω ReadHeader( const Xml::XMLElement& root, sv title, sv start={} )ε->string;
		β Lines()Ι->const google::protobuf::RepeatedPtrField<Edgar::Proto::IndexLine>& = 0;
		β Type()Ι->EForm = 0;
		β Cik()Ι->Cik=0;//{ return (Markets::Cik)IndexLine().cik(); }
		β PeriodEnd()Ι->Period=0;
	};
	inline Form::~Form(){}
}
#undef Φ