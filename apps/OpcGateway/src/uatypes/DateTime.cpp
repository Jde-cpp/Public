#include <jde/opc/uatypes/DateTime.h>
#include <jde/fwk/io/protobuf.h>

#define let const auto

namespace Jde::Opc{
	using namespace std::chrono;
	const UA_DateTime _ua1970 = UA_DateTime_fromUnixTime(0);

	Ω fromTimePoint( const TimePoint& tp )->UA_DateTime{
		return nanoseconds{ tp - Chrono::Epoch() }.count()/100+_ua1970;
	}

	UADateTime::UADateTime( const UA_DateTime& dt )ι:
#ifdef _MSC_VER
		_time{ Chrono::Epoch()+microseconds{(dt-_ua1970)/10} }
#else
		_time{ Chrono::Epoch()+nanoseconds{(dt-_ua1970)*100} }
#endif
	{}
	UADateTime::UADateTime( const jvalue& v, SL sl )ε{
		if( v.is_object() ){
			let& o = v.get_object();
			if( let* seconds = o.if_contains("seconds"); seconds && seconds->is_object() )
				_time = Jde::Protobuf::ToTimePoint( Protobuf::ToTimestamp(o) );
			else
				throw Exception{ (ELogTags)EOpcLogTags::Opc, sl, "Invalid DateTime object: {}", serialize(v) };
		}
		else if( v.is_string() )
			_time = Chrono::ToTimePoint( string{v.get_string()} );
		else if( v.is_number() )
			_time = Chrono::Epoch() + milliseconds{ v.to_number<_int>() };
		else
			throw Exception{ (ELogTags)EOpcLogTags::Opc, sl, "Invalid DateTime object: {}", serialize(v) };
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
	α UADateTime::ToParts()Ι->tuple<_int,int>{
		using namespace std::chrono;
		let dts = UA_DateTime_toStruct( fromTimePoint(_time) );
		_int seconds = Clock::to_time_t( Chrono::ToTimePoint((int16)dts.year, (int8)dts.month, (int8)dts.day, (int8)dts.hour, (int8)dts.min, (int8)dts.sec) );
		auto duration = milliseconds{dts.milliSec} + microseconds{dts.microSec} + nanoseconds{dts.nanoSec};
		int nanos = duration_cast<nanoseconds>(duration).count();
		return make_tuple( seconds, nanos );
	}
	α UADateTime::UA()Ι->UA_DateTime{
		return fromTimePoint( _time );
	}
}