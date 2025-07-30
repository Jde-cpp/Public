#pragma once
#ifndef DISK_WATCHER_H
#define DISK_WATCHER_H

#include "../threading/InterruptibleThread.h"
#include "../coroutine/Awaitable.h"
#include <shared_mutex>
struct inotify_event;
struct pollfd;
namespace Jde::IO
{
	using namespace Coroutine;
	enum class EDiskWatcherEvents : uint32_t
	{
		None         = 0x00000000,
		Access       = 0x00000001,	/* File was accessed */
		Modify       = 0x00000002,	/* File was modified */
		Attribute    = 0x00000004,	/* Metadata changed */
		CloseWrite   = 0x00000008,	/* Writtable file was closed */
		CloseNoWrite =	0x00000010,	/* Unwrittable file closed */
		Open         = 0x00000020,	/* File was opened */
		MovedFrom    = 0x00000040,	/* File was moved from X */
		MovedTo		 = 0x00000080,	/* File was moved to Y */
		Create		 = 0x00000100,	/* Subfile was created */
		Delete		 = 0x00000200,	/* Subfile was deleted */
		DeleteSelf	 =	0x00000400,	/* Self was deleted */
		MoveSelf		 = 0x00000800,	/* Self was moved */
											/* the following are legal events.  they are sent as needed to any watch */
											Unmount		 = 0x00002000,	/* Backing fs was unmounted */
											QOverflow	 =	0x00004000,	/* Event queued overflowed */
											Ignored		 = 0x00008000,	/* File was ignored */
																				/* special flags */
																				OnlyDir      =	0x01000000,	/* only watch the path if it is a directory */
																				DontFollow   = 0x02000000,	/* don't follow a sym link */
																				MaskAdd      = 0x20000000,	/* add to the mask of an already existing watch */
																				IsDir        = 0x40000000,	/* event occurred against dir */
																				OneShot      = 0x80000000,	/* only send event once */
																				AllEvents=Access+Modify+Attribute+CloseWrite+CloseNoWrite+Open+MovedFrom+MovedTo+Create+Delete+DeleteSelf+MoveSelf
	};
	constexpr inline EDiskWatcherEvents operator|(EDiskWatcherEvents a, EDiskWatcherEvents b){ return (EDiskWatcherEvents)( (uint32_t)a | (uint32_t)b ); }
	inline EDiskWatcherEvents operator&(EDiskWatcherEvents a, EDiskWatcherEvents b){ return (EDiskWatcherEvents)( (uint32_t)a & (uint32_t)b ); }
	inline bool Any(EDiskWatcherEvents a){ return a!=EDiskWatcherEvents::None; }
	struct NotifyEvent{
		NotifyEvent( const inotify_event& sys )ι;
		const int WatchDescriptor;
		const EDiskWatcherEvents Mask;
		const uint32_t Cookie;
		const string Name;
		α ToString()Ι->string;
	};

	struct Γ IDriveChange{
		β OnAccess( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnModify( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnAttribute( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnCloseWrite( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnCloseNoWrite( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnOpen( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnMovedFrom( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnMovedTo( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnCreate( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnDelete( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnDeleteSelf( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnMoveSelf( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnUnmount( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnQOverflow( const fs::path& path, const NotifyEvent& event )ι->void;
		β OnIgnored( const fs::path& path, const NotifyEvent& event )ι->void;
	};
//#endif
	struct WatcherSettings{
		Duration Delay;//{ 1min }
		vector<string> ExcludedRegEx;
	};
	enum EFileFlags
	{
		None = 0x0,
		Hidden = 0x2,
		System = 0x4,
		Directory = 0x10,
		Archive = 0x20,
		Temporary = 0x100
	};
	struct IDirEntry
	{
		IDirEntry()=default;
		IDirEntry( EFileFlags flags, const fs::path& path, uint size, const TimePoint& createTime=TimePoint(), const TimePoint& modifyTime=TimePoint() ):
			Flags{ flags },
			Path{ path },
			Size{ size },
			CreatedTime{ createTime },
			ModifiedTime{ modifyTime }
		{}
		virtual ~IDirEntry()
		{}

		bool IsDirectory()Ι{return Flags & EFileFlags::Directory;}
		EFileFlags Flags{EFileFlags::None};
		fs::path Path;
		uint Size{0};
		TimePoint AccessedTime;
		TimePoint CreatedTime;
		TimePoint ModifiedTime;
	};
	typedef sp<const IDirEntry> IDirEntryPtr;

	struct IDrive : std::enable_shared_from_this<IDrive>
	{
		β Recursive( const fs::path& path, SRCE )ε->flat_map<string,IDirEntryPtr> =0;
		β Get( const fs::path& path )ε->IDirEntryPtr=0;
		β Save( const fs::path& path, const vector<char>& bytes, const IDirEntry& dirEntry )ε->IDirEntryPtr=0;
		β CreateFolder( const fs::path& path, const IDirEntry& dirEntry )->IDirEntryPtr=0;
		β Remove( const fs::path& path )->void=0;
		β Trash( const fs::path& path )->void=0;
		β TrashDisposal( TimePoint latestDate )->void=0;
		β Load( const IDirEntry& dirEntry )->sp<vector<char>> =0;
		β Restore( sv name )ε->void=0;
		β SoftLink( const fs::path& existingFile, const fs::path& newSymLink )ε->void=0;
	};

	struct Γ DiskWatcher : std::enable_shared_from_this<DiskWatcher>
	{
		DiskWatcher( const fs::path& path, EDiskWatcherEvents events/*=DefaultEvents*/ )ε;
		virtual ~DiskWatcher();
		constexpr static EDiskWatcherEvents DefaultEvents = {EDiskWatcherEvents::Modify | EDiskWatcherEvents::MovedFrom | EDiskWatcherEvents::MovedTo | EDiskWatcherEvents::Create | EDiskWatcherEvents::Delete};
	protected:
		β OnModify( const fs::path& path, const NotifyEvent& /*event*/ )ι->void{ Warning( ELogTags::IO, "No listener for OnModify {}.", path.string() );};
		β OnMovedFrom( const fs::path& path, const NotifyEvent& /*event*/ )ι->void{ Warning( ELogTags::IO, "No listener for OnMovedFrom {}.", path.string() );};
		β OnMovedTo( const fs::path& path, const NotifyEvent& /*event*/ )ι->void{ Warning( ELogTags::IO, "No listener for OnMovedTo {}.", path.string() );};
		β OnCreate( const fs::path& path, const NotifyEvent& /*event*/ )ι->void=0;
		β OnDelete( const fs::path& path, const NotifyEvent& /*event*/ )ι->void{ Warning( ELogTags::IO, "No listener for OnDelete {}.", path.string() );};
	private:
		α Run()ι->void;
		α ReadEvent( const pollfd& fd, bool isRetry=false )ε->void;
		EDiskWatcherEvents _events{DefaultEvents};
		flat_map<uint32_t, fs::path> _descriptors;
		fs::path _path;
 		int _fd;
		sp<Jde::Threading::InterruptibleThread> _pThread;
	};
}
#endif