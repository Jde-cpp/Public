#include "Signals.h"
#include <cmath>
#include <numbers>
#include <random>

#define let const auto
namespace Jde::Opc::Emulator{
	constexpr array<sv,7> _modeNames{ "sine", "ramp", "randomWalk", "counter", "toggle", "command", "follow" };
	α ParseMode( sv s, SL sl )ε->EMode{
		for( uint i=0; i<_modeNames.size(); ++i ){
			if( _modeNames[i]==s )
				return (EMode)i;
		}
		THROWSL( "Unknown tag mode '{}' - one of {}.", s, Str::Join(_modeNames, "|") );
	}
	α ToString( EMode mode )ι->sv{ return _modeNames[underlying(mode)]; }

	TagSpec::TagSpec( const jobject& o, SL sl )ε:
		Name{ Json::AsString(o, "name", sl) },
		Mode{ ParseMode(Json::FindSV(o, "mode").value_or("counter"), sl) },
		Min{ Json::FindNumber<double>(o, "min").value_or(0) },
		Max{ Json::FindNumber<double>(o, "max").value_or(100) },
		Step{ Json::FindNumber<double>(o, "step").value_or(1) },
		RatedRpm{ Json::FindNumber<double>(o, "ratedRpm").value_or(1450) },
		Period{ Json::FindDuration(o, "period").value_or(30s) },
		Tau{ Json::FindDuration(o, "tau").value_or(3s) }{
		THROW_IFSL( Max<=Min && !IsBool() && Mode!=EMode::Follow, "Tag '{}': max ({}) must exceed min ({}).", Name, Max, Min );
		THROW_IFSL( Period<=Duration::zero() || Tau<=Duration::zero(), "Tag '{}': period and tau must be positive.", Name );
	}

	Ω seconds( Duration d )ι->double{ return std::chrono::duration<double>( d ).count(); }

	struct Sine final : IGenerator{
		Sine( const TagSpec& s )ι:_mid{ (s.Min+s.Max)/2 }, _amp{ (s.Max-s.Min)/2 }, _period{ seconds(s.Period) }{}
		α Next( Duration dt, bool )ι->double override{ _t += seconds(dt); return _mid + _amp*std::sin( 2*std::numbers::pi*_t/_period ); }
		double _mid, _amp, _period, _t{};
	};
	struct Ramp final : IGenerator{//sawtooth Min->Max over Period
		Ramp( const TagSpec& s )ι:_min{ s.Min }, _span{ s.Max-s.Min }, _period{ seconds(s.Period) }{}
		α Next( Duration dt, bool )ι->double override{ _t = std::fmod( _t+seconds(dt), _period ); return _min + _span*_t/_period; }
		double _min, _span, _period, _t{};
	};
	struct RandomWalk final : IGenerator{
		RandomWalk( const TagSpec& s )ι:_min{ s.Min }, _max{ s.Max }, _value{ (s.Min+s.Max)/2 }, _engine{ std::random_device{}() }, _dist{ -s.Step, s.Step }{}
		α Next( Duration, bool )ι->double override{ _value = std::clamp( _value+_dist(_engine), _min, _max ); return _value; }
		double _min, _max, _value; std::mt19937 _engine; std::uniform_real_distribution<double> _dist;
	};
	struct Counter final : IGenerator{//the soak's mode: +Step per cycle, wrapping at Max.
		Counter( const TagSpec& s )ι:_min{ s.Min }, _max{ s.Max }, _step{ s.Step }, _value{ s.Min }{}
		α Next( Duration, bool )ι->double override{ _value += _step; if( _value>_max ) _value = _min; return _value; }
		double _min, _max, _step, _value;
	};
	struct Toggle final : IGenerator{//bool flipping every Period
		Toggle( const TagSpec& s )ι:_period{ seconds(s.Period) }{}
		α Next( Duration dt, bool )ι->double override{ _t += seconds(dt); if( _t>=_period ){ _t = std::fmod(_t, _period); _on = !_on; } return _on ? 1 : 0; }
		double _period, _t{}; bool _on{ true };
	};
	struct Follow final : IGenerator{//a drive: first-order lag towards RatedRpm while commanded on, towards 0 when off.
		Follow( const TagSpec& s )ι:_rated{ s.RatedRpm }, _tau{ seconds(s.Tau) }{}
		α Next( Duration dt, bool command )ι->double override{
			let target = command ? _rated : 0.0;
			_value += ( target-_value )*( 1-std::exp(-seconds(dt)/_tau) );
			return _value;
		}
		double _rated, _tau, _value{};
	};

	α MakeGenerator( const TagSpec& spec )ε->up<IGenerator>{
		switch( spec.Mode ){
		case EMode::Sine: return mu<Sine>( spec );
		case EMode::Ramp: return mu<Ramp>( spec );
		case EMode::RandomWalk: return mu<RandomWalk>( spec );
		case EMode::Counter: return mu<Counter>( spec );
		case EMode::Toggle: return mu<Toggle>( spec );
		case EMode::Follow: return mu<Follow>( spec );
		case EMode::Command: THROW( "Tag '{}': command tags are subscribed, not generated.", spec.Name );
		}
		THROW( "Tag '{}': unhandled mode {}.", spec.Name, underlying(spec.Mode) );
	}
}
