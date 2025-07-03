#pragma once
#include "../threading/Interrupt.h"
#include "../collections/Map.h"
#include "DiskWatcher.h"

namespace Jde::IO
{
	struct DiskMirror : Jde::Threading::Interrupt, public IDriveChange
	{
		typedef std::chrono::steady_clock::time_point TimePoint;
		DiskMirror( path path );

		void OnTimeout()noexcept override;
		void OnAwake()noexcept override;
		void OnChange( path path )noexcept;
		fs::path GetRootTar( path path )noexcept;

	private:
		//map<fs::path, std::chrono::steady_clock::time_point> _changes; std::unique_mutex _changesMutex;
		//Collections::UnorderedMapValue<fs::path, TimePoint> _changes;
//		fs::path _foo;
		Duration _waitTime;
		Collections::MapValue<std::filesystem::path,TimePoint> _changes;
		//DiskWatcher _watcher;
	};
}