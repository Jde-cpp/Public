#pragma once

#ifdef _MSC_VER
	#pragma warning( push, 0  )
	#pragma warning( disable: 4005 )
		#define NTDDI_VERSION NTDDI_WIN10_RS1 // work around linker failure MapViewOfFileNuma2@36
	#pragma warning( pop )
#endif
#include <assert.h>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
//#include <coroutine>
#include <filesystem>

#include <optional>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <memory>

//#if !defined(__clang__) && !defined(_MSC_VER)
	#include <format>
//#else
//	#include <fmt/format.h>
//#endif
#include <coroutine>
#include <source_location>

#pragma warning(push)
#pragma warning( disable : 4715)
#include <nlohmann/json.hpp>
#pragma warning(pop)
using std::coroutine_handle;
using std::suspend_never;

#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#error WIN32_LEAN_AND_MEAN not defined
	#endif
	#define __PRETTY_FUNCTION__ __FUNCSIG__
	using std::stop_token;
#endif

#define DISABLE_WARNINGS _Pragma("warning( push, 0  )") _Pragma("warning( disable: 4702 )") _Pragma("warning( disable: 4715 )") _Pragma("warning( disable: 5105 )") _Pragma("warning( disable: 4701 )")  _Pragma("warning( disable: 5260 )")
#define ENABLE_WARNINGS  _Pragma("warning( pop  )")

DISABLE_WARNINGS
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
	#include <spdlog/fmt/ostr.h>
	#include <boost/container/flat_map.hpp>
	#include <boost/container/flat_set.hpp>
ENABLE_WARNINGS

#define α auto
#define β virtual auto
#define Ω static auto
#define Ξ inline auto
#define Τ template<class T>
#define Ŧ template<class T> auto
#define ẗ template<class K,class V> auto
#define Ṫ template<class T> static auto
#define ψ template<class... Args> auto
#define ι noexcept
#define Ι const noexcept
#define ε noexcept(false)
#define Ε const noexcept(false)
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
	using SessionPK=uint32;

	using Clock=std::chrono::system_clock;
	using Duration=Clock::duration;
	using TimePoint=Clock::time_point;
	using TP=Clock::time_point;

	using std::array;
	using std::atomic;
	using std::atomic_flag;
	using std::function;
	using lg = std::lock_guard<std::mutex>;
	//using std::make_unique;//refactor remove
	using std::make_shared;//refactor remove
	using std::mutex;
	Τ using sp = std::shared_ptr<T>;
	using std::string;
	using std::tuple;
	Τ using up = std::unique_ptr<T>;
	using std::get;
	using std::static_pointer_cast;
	using std::unique_lock;//refactor remove
	using ul=std::unique_lock<std::shared_mutex>;
	using sl=std::shared_lock<std::shared_mutex>;
	using std::shared_mutex;
	using sv = std::string_view;
	Τ using limits = std::numeric_limits<T>;
	using std::ranges::find;
	using std::ranges::find_if;
	using std::variant;
	using std::move;
	using std::endl;
	using std::optional;
	using std::ostringstream;
	using std::chrono::duration_cast;
	using std::make_tuple;
	using std::nullopt;

	template<class T, class... Args> α mu( Args&&... args )noexcept(noexcept(T(std::forward<Args>(args)...)))->up<T>{ static_assert(std::is_constructible_v<T,Args&&...>,"not constructable"); return up<T>( new T(std::forward<Args>(args)...) ); }
  template<class T, class... Args> α ms( Args&&... args )noexcept(noexcept(T(std::forward<Args>(args)...)))->sp<T>{ static_assert(std::is_constructible_v<T,Args&&...>,"not constructable"); return std::allocate_shared<T>( std::allocator<typename std::remove_const<T>::type>(), std::forward<Args>(args)... ); }

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
#ifdef _MSC_VER
	using std::format;
	using std::formatter;
	using std::format_parse_context;
#else
	using fmt::format;
	using fmt::formatter;
	using fmt::format_parse_context;
#endif
	using path = const fs::path&;
	using str = const std::string&;

	template<class T> using vec = const vector<T>&;

	using nlohmann::json;
	using std::coroutine_handle;
	using std::suspend_never;
	using std::source_location;
	#define SRCE_CUR std::source_location::current()
	#define SRCE const Jde::source_location& sl=SRCE_CUR
	using SL = const Jde::source_location&;

	using nlohmann::json;
#ifdef _MSC_VER
	inline constexpr bool _msvc{ true };
	#ifndef WIN32_LEAN_AND_MEAN
		#error WIN32_LEAN_AND_MEAN not defined
	#endif
	#define __PRETTY_FUNCTION__ __FUNCSIG__
#else
	constexpr bool _msvc{ false };
#endif

	enum class ELogLevel : int8{ NoLog=-1, Trace=0, Debug=1, Information=2, Warning=3, Error=4, Critical=5, None=6 };
	inline constexpr std::array<sv,7> ELogLevelStrings = { "Trace"sv, "Debug"sv, "Information"sv, "Warning"sv, "Error"sv, "Critical"sv, "None"sv };
	constexpr sv ToString( ELogLevel v )noexcept{ return (uint8)v<ELogLevelStrings.size() ? ELogLevelStrings[(uint8)v] : sv{}; }

#ifdef NDEBUG
	inline constexpr bool _debug{ false };
#else
	inline constexpr bool _debug{ true };
#endif
}