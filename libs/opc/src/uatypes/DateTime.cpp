#include <jde/opc/uatypes/DateTime.h>
#include <limits>
#include <jde/fwk/io/protobuf.h>
#include <jde/opc/uatypes/Logger.h>

#define let const auto

namespace Jde::Opc{
	using namespace std::chrono;
	const UA_DateTime _ua1970 = UA_DateTime_fromUnixTime(0);
	using UATick = duration<UA_Int64, std::ratio<1,10'000'000>>;//UA_DateTime's own unit: 100ns ticks since 1601.

	//The tick span that survives conversion into TimePoint's Duration.  duration_cast scales by
	//UATick::period/Duration::period, so a nanosecond clock (libstdc++) caps it at ~292 years either side of 1970, while
	//a 100ns (msvc stl) or microsecond (libc++) clock does not narrow it at all.  Deriving it beats the old #if on the
	//compiler: what matters is the clock's resolution, not who compiled it.
	using TickRatio = std::ratio_divide<UATick::period, Duration::period>;
	constexpr UA_Int64 _maxTicks = TickRatio::num>TickRatio::den //(numeric_limits<>::max) parenthesized: windows.h's max macro is in scope through open62541.
		? (UA_Int64)( (std::numeric_limits<Duration::rep>::max)()/(TickRatio::num/TickRatio::den) )
		: (std::numeric_limits<UA_Int64>::max)();
	constexpr UA_Int64 _minTicks = -_maxTicks;

	//Clamp rather than wrap.  An extreme UA_DateTime - INT64_MIN is a common "no value" sentinel - used to overflow in
	//`dt - _ua1970` before any conversion saw it, so INT64_MIN and INT64_MAX both came back as the same 2185-ish date.
	Ω toTicks( UA_DateTime dt )ι->UA_Int64{
		if( dt<0 && dt<std::numeric_limits<UA_Int64>::min()+_ua1970 )//the subtraction itself would wrap.
			return _minTicks;
		let ticks = dt-_ua1970;
		return ticks<_minTicks ? _minTicks : ticks>_maxTicks ? _maxTicks : ticks;
	}

	Ω fromTimePoint( const TimePoint& tp )ι->UA_DateTime{
		return duration_cast<UATick>( tp-Chrono::Epoch() ).count()+_ua1970;
	}

	UADateTime::UADateTime( const UA_DateTime& dt )ι:
		_time{ Chrono::Epoch()+duration_cast<Duration>(UATick{toTicks(dt)}) }
	{}
	constexpr _int _maxSeconds = (std::numeric_limits<Duration::rep>::max)()/(Duration::period::den/Duration::period::num);

	UADateTime::UADateTime( const jvalue& v, SL sl )ε{
		let at = [&]( _int s, _int ns )ε->TimePoint{
			if( s>_maxSeconds || s<-_maxSeconds )
				throw Exception{ sl, (ELogTags)EOpcLogTags::Opc, "DateTime seconds {} is outside the ±{} a TimePoint holds: {}", s, _maxSeconds, serialize(v) };
			return Chrono::Epoch()+duration_cast<Duration>( seconds{s} )+duration_cast<Duration>( nanoseconds{ns} );
		};
		if( v.is_object() ){
			let& o = v.get_object();
			let* secondsPtr = o.if_contains( "seconds" );
			if( secondsPtr && secondsPtr->is_object() ) //protobufjs Long form {high,low}
				_time = Jde::Protobuf::ToTimePoint( Protobuf::ToTimestamp(o) );
			else if( secondsPtr && secondsPtr->is_number() ){ //plain numbers, as ToJson emits
				let* nanos = o.if_contains( "nanos" );
				_time = at( secondsPtr->to_number<int64_t>(), nanos && nanos->is_number() ? nanos->to_number<int64_t>() : 0 );
			}
			else
				throw Exception{ sl, {(ELogTags)EOpcLogTags::Opc}, "Invalid DateTime object: {}", serialize(v) };
		}
		else if( v.is_string() )
			_time = Chrono::ToTimePoint( string{v.get_string()} );
		else if( v.is_number() ){
			let ms = v.to_number<_int>();//a bare number is milliseconds since the epoch.
			_time = at( ms/1000, (ms%1000)*1'000'000 );
		}
		else
			throw Exception{ sl, {(ELogTags)EOpcLogTags::Opc}, "Invalid DateTime object: {}", serialize(v) };
	}
	UADateTime::UADateTime( const google::protobuf::Timestamp& timestamp )ι:
		_time{ Jde::Protobuf::ToTimePoint( timestamp ) }
	{}

	UADateTime::UADateTime( const google::protobuf::Duration& duration )ι:
		_time{ Chrono::Epoch() + milliseconds{ duration.seconds() * 1000 + duration.nanos() / 1000000 } }
	{}

	α UADateTime::ToJson()Ι->jobject{
		let [seconds,nanos] = ToParts();
		return jobject{ {"seconds", seconds}, {"nanos", nanos} };
	}
	α UADateTime::ToProto()Ι->google::protobuf::Timestamp{
		let [seconds,nanos] = ToParts();
		google::protobuf::Timestamp t;
		t.set_seconds( seconds );
		t.set_nanos( nanos );
		return t;
	}
	α UADateTime::ToDuration()Ι->google::protobuf::Duration{
		let ts = ToProto();
		google::protobuf::Duration d; d.set_seconds( ts.seconds() ); d.set_nanos( ts.nanos() );
		return d;
	}
	//A duration split, not a calendar round trip.  Going out through UA_DateTime_toStruct and back through
	//Chrono::ToTimePoint called an ε function from this Ι one - a pre-year-0 year wrapped into its uint16_t parameter and
	//THROW_IFSL(!ymd.ok()) then escaped a noexcept frame - and it cost a full civil-time conversion to learn nothing the
	//duration did not already say.  floor, not duration_cast: nanos has to stay in [0,1e9) for a protobuf Timestamp, and
	//truncation toward zero would make it negative before 1970.
	α UADateTime::ToParts()Ι->tuple<_int,int>{
		let sinceEpoch = _time-Chrono::Epoch();
		let secs = floor<seconds>( sinceEpoch );
		return make_tuple( (_int)secs.count(), (int)duration_cast<nanoseconds>(sinceEpoch-secs).count() );
	}
	α UADateTime::UA()Ι->UA_DateTime{
		return fromTimePoint( _time );
	}
}