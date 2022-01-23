#pragma once

#ifdef _MSC_VER
	#define NTDDI_VERSION NTDDI_WIN10_RS1 // work around linker failure MapViewOfFileNuma2@36
#endif
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <filesystem>

#include <optional>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <memory>

#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#error WIN32_LEAN_AND_MEAN not defined
	#endif
	#include <coroutine>
	#include <source_location>
	#define __PRETTY_FUNCTION__ __FUNCSIG__
	using std::coroutine_handle;
	using std::suspend_never;
	using std::stop_token;
#else
	#include <experimental/coroutine>
	#include <boost/assert/source_location.hpp>
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;
	//using Jde::stop_token;
#endif

#define DISABLE_WARNINGS _Pragma("warning( push, 0  )") _Pragma("warning( disable: 4702 )") _Pragma("warning( disable: 4715 )") _Pragma("warning( disable: 5105 )") _Pragma("warning( disable: 4701 )")
#define ENABLE_WARNINGS  _Pragma("warning( pop  )")

#ifndef NO_FORMAT
	DISABLE_WARNINGS
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
	ENABLE_WARNINGS
#endif

#ifndef NO_BOOST
	#include <boost/container/flat_map.hpp>
	#include <boost/container/flat_set.hpp>
#endif

#define α auto
#define β virtual auto
#define Ω static auto
#define Ξ inline auto
#define Τ template<class T>
#define ⓣ template<class T> auto
#define ẗ template<class K,class V> auto
#define Ṫ template<class T> static auto
#define ψ template<class... Args> auto
//#define Φ Γ auto
namespace Jde
{
	using namespace std::literals::string_view_literals;

	using uint8=uint_fast8_t;
	using int8=int_fast8_t;

	using uint16=uint_fast16_t;
	using int16=int_fast16_t ;

	using uint32=uint_fast32_t;
	using PK=uint_fast64_t;
	using UserPK=PK;
	using int32=int_fast32_t;

	using uint=uint_fast64_t;
	using _int=int_fast64_t;
	using Handle=uint;

	using Clock=std::chrono::system_clock;
	using Duration=Clock::duration;
	using TimePoint=Clock::time_point;
	using TP=Clock::time_point;
	/*
	using SDuration=SClock::duration;
	using STimePoint=SClock::time_point;*/

	using std::array;
	using std::atomic;
	using std::atomic_flag;
	using std::function;
	using std::lock_guard;
	using std::make_unique;//refactor remove
	using std::make_shared;//refactor remove
	using std::mutex;
	template<class T> using sp = std::shared_ptr<T>;
	using std::string;
	using std::tuple;
	template<class T> using up = std::unique_ptr<T>;
	using std::get;
	using std::static_pointer_cast;
	using std::unique_lock;//refactor remove
	using ul=std::unique_lock<std::shared_mutex>;
	using std::shared_lock;//refactor remove
	using sl=std::shared_lock<std::shared_mutex>;
	using std::shared_mutex;
	using sv = std::string_view;
	using std::find;
	using std::find_if;
	using std::move;
	using std::endl;
	using std::optional;
	using std::ostringstream;
	using std::chrono::duration_cast;
	using std::make_tuple;
	using std::nullopt;

	template<class T, class... Args> α mu( Args&&... args )->up<T>{ return up<T>( new T(std::forward<Args>(args)...) ); }
  	template<class T, class... Args> α ms( Args&&... args ){ static_assert(std::is_constructible_v<T,Args&&...>,"not constructable"); return std::allocate_shared<T>( std::allocator<typename std::remove_const<T>::type>(), std::forward<Args>(args)... ); }

	using std::vector;
	template<class T> using VectorPtr = std::shared_ptr<std::vector<T>>;

	using PortType=unsigned short;
	using DayIndex=uint_fast16_t;//TODO Refactor remove
	using Day=uint_fast16_t;

	namespace fs=std::filesystem;
#ifndef NO_BOOST
	using boost::container::flat_map;
	using boost::container::flat_multimap;
	using boost::container::flat_set;
#endif
	using fmt::format;
	using path = const fs::path&;
	using str = const std::string&;
	template<class T> using vec = const vector<T>&;

#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#error WIN32_LEAN_AND_MEAN not defined
	#endif
	#define __PRETTY_FUNCTION__ __FUNCSIG__
	using std::coroutine_handle;
	using std::suspend_never;
	using std::source_location;
	#define SRCE_CUR std::source_location::current()
#else
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;
	using boost::source_location;
	#define SRCE_CUR boost::source_location{ __builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION() }
#endif
	#define SRCE const source_location& sl=SRCE_CUR
	using SL = const source_location&;


	enum class ELogLevel : int8{ NoLog=-1, Trace=0, Debug=1, Information=2, Warning=3, Error=4, Critical=5, None=6 };
	constexpr std::array<sv,7> ELogLevelStrings = { "Trace"sv, "Debug"sv, "Information"sv, "Warning"sv, "Error"sv, "Critical"sv, "None"sv };
	constexpr sv ToString( ELogLevel v )noexcept{ return (uint8)v<ELogLevelStrings.size() ? ELogLevelStrings[(uint8)v] : sv{}; }
}