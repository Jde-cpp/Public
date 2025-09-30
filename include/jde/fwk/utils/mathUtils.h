#pragma once
#include <numeric>
#include <random>
#include <ranges>

namespace Jde
{
	template<typename T=uint> α Round( double value )->T
	{
		return static_cast<T>( llround(value) );
	}
}
namespace Jde::Math
{
	Τ struct StatResult
	{
		T Average{0.0};
		T Variance{0.0};
		T Min{0.0};
		T Max{0.0};
	};

  static up<std::mt19937> _engine;
	Ξ Random()->uint32
	{
		if( !_engine )
		{
			_engine = mu<std::mt19937>();
//#ifdef NDEBUG
	    std::random_device rd;
	    auto rd_range = std::ranges::transform_view(std::ranges::iota_view(static_cast<std::size_t>(0), std::mt19937::state_size), [&rd](size_t){return rd();});
	    std::seed_seq seeds(rd_range.begin(), rd_range.end());
			_engine->seed( seeds );
//#endif
		}
		return (*_engine)();
	}

	template<typename T=double> struct Point
	{
		α Distance( Point<T> o )Ι->T{ return std::pow( std::pow(o.X-X,2.0)+std::pow(o.Y-Y,2) ,.5); }
		T X{0.0};
		T Y{0.0};
	};

#define let const auto
	Ŧ Statistics( const T& values, bool calcVariance=true )ι->StatResult<typename T::value_type>
	{
		typedef typename T::value_type TValue;
		let size = values.size();
		//ASSERT( size>0 );
		TValue sum{};
		TValue min{ std::numeric_limits<TValue>::max() };
		TValue max{ std::numeric_limits<TValue>::min() };
		TValue average{};
		TValue variance{};
		for( let& value : values )
		{
			sum += value;
			min = std::min( min, value );
			max = std::max( max, value );
		}
		if( size>0 )
		{
			average = sum/size;
			if( size>1 && calcVariance )
			{
				auto varianceFunction = [&average, &size]( double accumulator, const double& val )
				{
					let diff = val - average;
					return accumulator + diff*diff / (size - 1);//sample?
				};
				double v2 = std::accumulate( values.begin(), values.end(), 0.0, varianceFunction );
				variance = static_cast<TValue>( v2 );
			}
		}
		return StatResult<TValue>{ average, variance, min, max };
	}
#undef let
}