#pragma once
#include <assert.h>
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include <coroutine>
#include <source_location>

#if _MSC_VER
	#define DISABLE_WARNINGS _Pragma("warning(push, 0)") _Pragma("warning(disable: 4244)")  _Pragma("warning(disable: 4701)") _Pragma("warning(disable: 4702)") _Pragma("warning(disable: 4715)")  _Pragma("warning(disable: 5054)") _Pragma("warning(disable: 5104 )") _Pragma("warning(disable: 5105 )") _Pragma("warning(disable: 5260)")
	#define ENABLE_WARNINGS  _Pragma("warning( pop )")
#else
	#define DISABLE_WARNINGS
	#define ENABLE_WARNINGS
#endif

#include <boost/json.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>

#define SPDLOG_FMT_EXTERNAL
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/basic_file_sink.h>
#ifndef NDEBUG
	#define BOOST_USE_ASAN
#endif
ENABLE_WARNINGS

#ifdef _MSC_VER
	#pragma warning( push, 0  )
	#pragma warning( disable: 4005 )
		#define NTDDI_VERSION NTDDI_WIN10_RS1 // work around linker failure MapViewOfFileNuma2@36
	#pragma warning( pop )
#endif
#ifdef _MSC_VER
	#ifndef WIN32_LEAN_AND_MEAN
		#error WIN32_LEAN_AND_MEAN not defined
	#endif
	#define __PRETTY_FUNCTION__ __FUNCSIG__
	using std::stop_token;
#endif

#ifdef _MSC_VER
	inline constexpr bool _msvc{ true };
	#define __PRETTY_FUNCTION__ __FUNCSIG__
	#ifdef _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
#else
	constexpr bool _msvc{ false };
#endif
#include "framework/macros.h"
#include "framework/usings.h"
#include "framework/enum.h"
#include "framework/exceptions/assert.h"
#include "framework/exports.h"
#include "framework/log/Logger.h"
#include "framework/exceptions/Exception.h"