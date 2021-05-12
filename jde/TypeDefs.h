#pragma once
#ifndef JDE_TYPEDEFS
#define JDE_TYPEDEFS

#ifndef PROTOBUF_USE_LIB
//	#define PROTOBUF_USE_DLLS
//	#define PROTOBUF_USE_EXCEPTIONS 0
#endif

#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <list>

#include <optional>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <string_view>
#include <vector>

#define DISABLE_WARNINGS _Pragma("warning( push, 0  )") _Pragma("warning( disable: 4702 )") _Pragma("warning( disable: 4715 )") _Pragma("warning( disable: 5105 )")
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
#endif

#ifdef _MSC_VER
	#if __cplusplus > 2017
		#include <coroutine>
	#else
		#include <experimental/coroutine>
	#endif
#else
	#include <experimental/coroutine>
#endif

#ifdef ONE_CORE
	#pragma comment(lib, "Onecore.lib")
#endif

namespace Jde
{
	typedef uint_fast8_t uint8;
	typedef int_fast8_t int8;

	using namespace std::literals::string_view_literals;

#pragma region ELogLevel
	enum class ELogLevel : uint8
	{
		Trace = 0,
		Debug = 1,
		Information = 2,
		Warning = 3,
		Error = 4,
		Critical = 5,
		None = 6
	};
	constexpr std::array<std::string_view,7> ELogLevelStrings = { "Trace"sv, "Debug"sv, "Information"sv, "Warning"sv, "Error"sv, "Critical"sv, "None"sv };
#pragma endregion

	template<typename T>
	constexpr auto ms = std::make_shared<T>;

	template<typename T>
	constexpr auto mu = std::make_unique<T>;

	typedef uint_fast16_t uint16;
	typedef int_fast16_t int16;

	typedef uint_fast32_t uint32;
	typedef uint_fast64_t PK;
	typedef PK UserPK;
	typedef int_fast32_t int32;

	using uint=uint_fast64_t;
	typedef int_fast64_t _int;
	typedef uint Handle;

	typedef std::chrono::system_clock Clock;
	typedef Clock::duration Duration;
	typedef Clock::time_point TimePoint;

	typedef std::chrono::steady_clock SClock;
	typedef SClock::duration SDuration;
	typedef SClock::time_point STimePoint;

	using std::array;
	using std::lock_guard;
	using std::make_unique;
	using std::make_shared;
	using std::mutex;
	using std::shared_ptr;
	using std::string;
	using std::tuple;
	using std::unique_ptr;
	using std::get;
	using std::set;
	using std::static_pointer_cast;
	using std::unique_lock;
	using std::shared_lock;
	using std::shared_mutex;
	using std::find;
	using std::find_if;
	using std::move;
	using std::endl;
	using std::optional;
	using std::ostringstream;
	using std::chrono::duration_cast;
	using std::make_tuple;
	using std::list;
	using std::atomic;
	//using boost::container::flat_multimap;

	template<class T> using sp = std::shared_ptr<T>;
	template<class T> using up = std::unique_ptr<T>;

	using std::vector;
	template<class T> using VectorPtr = std::shared_ptr<std::vector<T>>;

	using std::map;
	template<class K, class V> using MapPtr = std::shared_ptr<std::map<K,V>>;
	template<class K, class V> using UMapPtr = std::unique_ptr<std::map<K,V>>;
	template<class K, class V> using UnorderedPtr = std::shared_ptr<std::unordered_map<K,V>>;
	using std::multimap;
	template<class T, class Y> using MultiMapPtr = std::shared_ptr<std::multimap<T,Y>>;
	using std::function;

	typedef unsigned short PortType;
	typedef uint_fast16_t DayIndex;

	namespace fs=std::filesystem;
#ifndef NO_BOOST
	using boost::container::flat_map;
	using boost::container::flat_multimap;
#endif
#ifndef NO_FORMAT
	using fmt::format;
	#ifdef _MSC_VER
		#ifdef NDEBUG
			#pragma comment(lib, "fmt.lib")
		#else
			#pragma comment(lib, "fmtd.lib")
		#endif
	#endif
#endif
	using path = const fs::path&;
	using sv = std::string_view;
	using str = const std::string&;

#ifdef _MSC_VER
	#define __PRETTY_FUNCTION__ __FUNCSIG__
	#if __cplusplus > 2017
		using std::coroutine_handle;
		using std::suspend_never;
	#else
		using std::experimental::coroutine_handle;
		using std::experimental::suspend_never;
	#endif
#else
	using std::experimental::coroutine_handle;
	using std::experimental::suspend_never;
#endif
}

#define α auto
#define ρ friend auto
#define Ω static auto
#define ψ template<> inline auto
//#define ➡️ inline auto
#define ⓣ template<typename T> auto

//📋 Write( )->uint{ return 0; }

#endif // !JDE_TYPEDEFS