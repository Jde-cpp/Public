//https://www.cise.ufl.edu/~seeger/dist/tzdump.c
#include <arpa/inet.h>
#include "../../Framework/source/Cache.h"
#include "../../Framework/source/DateTime.h"
#include <fstream>

#define var const auto
namespace Jde
{
	constexpr sv Magic = "TZif"sv;
	struct tzhead
	{
		char	tzh_magic[4];		/* TZ_MAGIC */
		char	tzh_version[1];		/* '\0' or '2' or '3' as of 2013 */
		char	tzh_reserved[15];	/* reserved; must be zero */
		char	tzh_ttisutcnt[4];	/* coded number of trans. time flags */
		char	tzh_ttisstdcnt[4];	/* coded number of trans. time flags */
		char	tzh_leapcnt[4];		/* coded number of leap seconds */
		uint32_t tzh_timecnt;		/* coded number of transition times */
		uint32_t tzh_typecnt;		/* coded number of local time types */
		char	tzh_charcnt[4];		/* coded number of abbr. chars */
	};
	typedef flat_map<TimePoint,Duration, std::greater<TimePoint>> CacheType;
	sp<CacheType> LoadGmtOffset( sv name )noexcept(false)
	{
		if( name=="EST (Eastern Standard Time)" )
			name = "EST5EDT";
		else if( name=="MET (Middle Europe Time)" )
			name = "MET";
		const fs::path path{ fs::path{"/usr/share/zoneinfo"}/fs::path{name} }; CHECK_PATH( path, SRCE_CUR );

		std::ifstream is{ path.string() };
		tzhead head;
		is.read(  (char*)&head, sizeof(tzhead) );
		THROW_IFX( string(head.tzh_magic,4)!=string(Magic), IO_EX(path, ELogLevel::Error, "Magic not equal '{}'", string(head.tzh_magic,4)) );
		THROW_IFX( is.bad(), IOException(path,"after header is.bad()") );

		var count = ntohl( head.tzh_timecnt );
		std::vector<TimePoint> transistionTimes; transistionTimes.reserve( count );
		for( uint32_t i=0;i<count; ++i )
		{
			int32_t value;
			is.read( (char*)&value, sizeof(value) );
			transistionTimes.push_back( Clock::from_time_t(ntohl(value)) );
		}
		std::map<TimePoint, uint8> timeTypes;
		for( uint32_t i=0;i<count; ++i )
			timeTypes.try_emplace( timeTypes.end(), transistionTimes[i], is.get() );

		var typeCount = ntohl( head.tzh_typecnt );
		vector<int32_t> typeOffsets; typeOffsets.reserve( typeCount );
		for( uint32_t i = 0; i < typeCount; ++i )
		{
			int32_t value;
			is.read( (char*)&value, sizeof(value) );
			int32_t value2 = ntohl( value );
			typeOffsets.push_back( value2 );
			/*unsigned char isDst =*/ is.get();
			/*unsigned char abbreviationIndex = */ is.get();
		}
		THROW_IFX( is.bad(), IOException(path,"is.bad()") );

		auto pResults = make_shared<CacheType>();
		for( var [time,type] : timeTypes )
		{
			THROW_IFX( type>=typeOffsets.size(), IO_EX(path, ELogLevel::Error, "type>=typeOffsets.size() {}>={}", type, typeOffsets.size()) );
			pResults->emplace( time, std::chrono::seconds(typeOffsets[type]) );
		}
		return pResults;
	}

	Duration Timezone::EasternTimezoneDifference( TimePoint utc, SL sl )noexcept(false)
	{
		return TryGetGmtOffset( "EST5EDT", utc, sl );
	}

	Duration Timezone::GetGmtOffset( sv name, TimePoint utc, SL sl )noexcept(false)
	{
		var key = Jde::format( "GetGmtOffset-{}", name );
		auto pInfo = Cache::Emplace<CacheType>( key );
		if( !pInfo || !pInfo->size() )
			Cache::Set<CacheType>( key, pInfo = LoadGmtOffset( name ) );
		var pStartDuration = pInfo->lower_bound( utc );
		if( pStartDuration==pInfo->end() ) throw Jde::Exception{ sl, ELogLevel::Error, "No info for '{}'", ToIsoString(utc) };
		const Duration value{ pStartDuration->second };
		return value;
	}
}