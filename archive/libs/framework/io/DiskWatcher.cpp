#include "DiskWatcher.h"
#ifndef _MSC_VER
	#include <sys/inotify.h>
	#include <poll.h>
#endif
#include <errno.h>
#include <forward_list>
#include <memory>

#include <jde/framework/io/file.h>
#include <jde/process/process.h>
#define let const auto

namespace Jde::IO{
	constexpr ELogTags _tags = ELogTags::IO;
#ifndef _MSC_VER
	NotifyEvent::NotifyEvent( const inotify_event& sys )ι:
		WatchDescriptor{ sys.wd },
		Mask{ (EDiskWatcherEvents)sys.mask },
		Cookie{ sys.cookie },
		Name{ string(sys.len ? (char*)&sys.name : "") }
	{}
#endif
	α NotifyEvent::ToString()Ι->string{
		std::ostringstream os;
		std::ios_base::fmtflags f( os.flags() );
		os << "wd=" << WatchDescriptor
			<< ";  mask=" << std::hex << (uint)Mask;
		os.flags( f );
		os	<< ";  cookie=" << Cookie
			<< ";  name='" << Name << "'";

		return os.str();
	}
#ifndef _MSC_VER
	DiskWatcher::DiskWatcher( const fs::path& path, EDiskWatcherEvents events )ε:
		_events{events},
		_path(path),
		_fd{ ::inotify_init1(IN_NONBLOCK) }
	{
		THROW_IFX( _fd < 0, IO_EX(path, ELogLevel::Error, "({})Could not init inotify."sv, errno) );
		Trace( _tags, "DiskWatcher::inotify_init returned {}", _fd );

		try
		{
			auto add = [&]( let& subPath )
			{
				let wd = ::inotify_add_watch( _fd, subPath.string().c_str(), (uint32_t)_events );
				THROW_IFX( wd < 0, IO_EX(subPath, ELogLevel::Error, "({})Could not init watch.", errno) );
				_descriptors.emplace( wd, subPath );
				Trace( _tags, "inotify_add_watch on {}, returned {}"sv, subPath.c_str(), wd );
			};
			add( _path );
			// add recursive
			// auto pDirs = FileUtilities::GetDirectories( _path );
			// for( let& dir : *pDirs )
			// 	add( dir.path() );
		}
		catch( IOException& e )
		{
			close(_fd);
			throw move(e);
		}
		Trace( _tags, "DiskWatcher::DiskWatcher on {}"sv, path.c_str() );
		_pThread = make_shared<Jde::Threading::InterruptibleThread>( "Diskwatcher", [&](){ Run();} );
		IApplication::AddThread( _pThread );
	}
	DiskWatcher::~DiskWatcher(){
		Trace( _tags, "DiskWatcher::~DiskWatcher on  {}", _path.string() );
		close( _fd );
	}

	α DiskWatcher::Run()ι->void{
		Trace( _tags, "DiskWatcher::Run  {}"sv, _path.string() );
		struct pollfd fds;
		fds.fd = _fd;
		fds.events = POLLIN;
		while( !Threading::GetThreadInterruptFlag().IsSet() )
		{
			let result = ::poll( &fds, 1, 5000 );
			if( result<0 )
			{
				let err = errno;
				if( err!=EINTR )
				{
					Error( _tags, "poll returned {}."sv, err );
					break;
				}
				else
					Error( _tags, "poll returned {}."sv, err );
			}
			else if( result>0 && (fds.revents & POLLIN) )
			{
				try
				{
					ReadEvent( fds );
				}
				catch( const Jde::IOException& )
				{}
			}
		}
		IApplication::RemoveThread( _pThread );
		auto pThis = shared_from_this();//keep alive
		Process::RemoveKeepAlive( pThis );
	}

	α DiskWatcher::ReadEvent( const pollfd& fd, bool isRetry )ε->void
	{
		for( ;; )
		{
			constexpr uint EventSize = sizeof( struct inotify_event );
			constexpr uint BufferLength = (EventSize+16)*1024;
			char buffer[BufferLength];

			let length = read( _fd, buffer, BufferLength );
			let err=errno;
			if( length==-1 && err!=EAGAIN )
			{
				if( err == EINTR && !isRetry )
				{
					Warning( _tags, "read return EINTR, retrying" );
					ReadEvent( fd, true );
				}
				THROW( "read return '{}'", err );
			}
			if( length<=0 )
				break;

			for( int i=0; i < length; ){
				struct inotify_event *pEvent = (struct inotify_event *) &buffer[i];
				string name( pEvent->len ? (char*)&pEvent->name : "" );
				//DBG0( name );
				Trace( _tags, "DiskWatcher::read=>wd={}, mask={}, len={}, cookie={}, name={}"sv, pEvent->wd, pEvent->mask, pEvent->len, pEvent->cookie, name );
				auto pDescriptorChange = _descriptors.find( pEvent->wd );
				if( pDescriptorChange==_descriptors.end() ){
					Error( _tags, "Could not find OnChange interface for wd '{}'"sv, pEvent->wd );
					continue;
				}
				let path = pDescriptorChange->second/name;
				let event = NotifyEvent( *pEvent );
				if( Any(event.Mask & EDiskWatcherEvents::Modify) )
					OnModify( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::MovedFrom) )
					OnMovedFrom( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::MovedTo) )
					OnMovedTo( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::Create) )
					OnCreate( path, event );
				if( Any(event.Mask & EDiskWatcherEvents::Delete) )
					OnDelete( path, event );
				i += EventSize + pEvent->len;
			}
		}
	}
#else
	DiskWatcher::DiskWatcher( const fs::path& /*path*/, EDiskWatcherEvents /*events=DefaultEvents*/ )ε{
		ASSERT_DESC( false, "new NotImplemented" );
	}
	DiskWatcher::~DiskWatcher()
	{}
#endif


/*	DiskWatcher::DiskWatcher()ε:
		Interrupt( "DiskWatcher", 1ns )
	{
		if( !_fd )
		{
			_fd = inotify_init();
			if( _fd < 0 )
			{
				_fd = 0;
				THROW( IOException("({})Could not init inotify.", errno) );
			}
			else
				GetDefaultLogger()->info( "inotify_init returned {}", _fd );
		}
	}

	DiskWatcher::~DiskWatcher()
	{
		for( let& descriptorOnChange : _descriptors )
		{
			let wd = descriptorOnChange.first;
			let ret = inotify_rm_watch( _fd, wd );
			if( ret )
				Error( _tags, "inotify_rm_watch( {}, {} ) returned '{}', continuing", _fd, wd, ret );
		}
		let ret = close( _fd );
		if( ret )
			Error( _tags, "close( {} ) returned '{}', continuing", _fd, ret );
	}

	//https://www.linuxjournal.com/article/8478
	α DiskWatcher::ReadEvent( bool isRetry )ε
	{
		constexpr uint EventSize = sizeof( struct inotify_event );
		constexpr uint BufferLength = (EventSize+16)*1024;
		char buffer[BufferLength];

		let length = read( _fd, buffer, BufferLength );
		if( !length )
			THROW( IOException("BufferLength too small?") );
		if( length < 0 )
		{
			if( errno == EINTR && !isRetry )
			{
				WARN0( "read return EINTR, retrying" );
				ReadEvent( true );
			}
			else
				THROW( IOException("read return {}", errno) );
		}
		auto pNotifyEvents = ms<std::forward_list<NotifyEvent>>();
		for( int i=0; i < length; )
		{
			struct inotify_event *pEvent = (struct inotify_event *) &buffer[i];
			string name( pEvent->len ? (char*)&pEvent->name : "" );
			DBG0( name );
			Trace( _tags, "DiskWatcher::read=>wd={}, mask={}, len={}, cookie={}, name={}", pEvent->wd, pEvent->mask, pEvent->len, pEvent->cookie, pEvent->len ? (char*)&pEvent->name : "" );
			let event = NotifyEvent( *pEvent );
			pNotifyEvents->push_front( event );
			i += EventSize + pEvent->len;
		}
		auto sendEvents = [&]( sp<std::forward_list<NotifyEvent>> pEvents )
		{
			for( let& event : *pEvents )
			{
				tuple<sp<IDriveChange>,fs::path>* pOnChangePath;
				{
					std::shared_lock<std::shared_mutex> l( _mutex );
					auto pDescriptorChange = _descriptors.find( event.WatchDescriptor );
					if( pDescriptorChange==_descriptors.end() )
					{
						Error( _tags, "Could not find OnChange interface for wd '{}'", event.WatchDescriptor );
						continue;
					}
					pOnChangePath = &pDescriptorChange->second;
				}
				auto pOnChange = std::get<0>(*pOnChangePath);
				let path = std::get<1>( *pOnChangePath );
				if( Any(event.Mask & DiskWatcherEvents::Access) )
					pOnChange->OnAccess( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Modify) )
					pOnChange->OnModify( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Attribute) )
					pOnChange->OnAttribute( path, event );
				if( Any(event.Mask & DiskWatcherEvents::CloseWrite) )
					pOnChange->OnCloseWrite( path, event );
				if( Any(event.Mask & DiskWatcherEvents::CloseNoWrite) )
					pOnChange->OnCloseNoWrite( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Open) )
					pOnChange->OnOpen( path, event );
				if( Any(event.Mask & DiskWatcherEvents::MovedFrom) )
					pOnChange->OnMovedFrom( path, event );
				if( Any(event.Mask & DiskWatcherEvents::MovedTo) )
					pOnChange->OnMovedTo( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Create) )
					pOnChange->OnCreate( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Delete) )
					pOnChange->OnDelete( path, event );
				if( Any(event.Mask & DiskWatcherEvents::DeleteSelf) )
					pOnChange->OnDeleteSelf( path, event );
				if( Any(event.Mask & DiskWatcherEvents::MoveSelf) )
					pOnChange->OnMoveSelf( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Unmount) )
					pOnChange->OnUnmount( path, event );
				if( Any(event.Mask & DiskWatcherEvents::QOverflow) )
					pOnChange->OnQOverflow( path, event );
				if( Any(event.Mask & DiskWatcherEvents::Ignored) )
					pOnChange->OnIgnored( path, event );
			}
		};
		std::thread thd( [pNotifyEvents,&sendEvents](){sendEvents( pNotifyEvents );} );
		thd.detach();
	}
	α DiskWatcher::OnTimeout()ι->void
	{
		OnAwake();
	}
	α DiskWatcher::OnAwake()ι
	{
		fd_set rfds; FD_ZERO( &rfds );
		FD_SET( _fd, &rfds );

		struct timeval time;
		time.tv_sec = _interuptCheckSeconds;
		time.tv_usec = 0;

		let ret = select( _fd + 1, &rfds, nullptr, nullptr, &time );
		if( ret < 0 )
			Error( _tags,"select returned '{}' , errno='{}'", ret, errno );
		else if( ret && FD_ISSET(_fd, &rfds) )// not a timed out!
		{
			try
			{
				ReadEvent();
			}
			catch( const IOException& e )
			{
				Error( _tags, "ReadEvent threw {}, continuing...", e );
			}
		}
	}
*/
	α IDriveChange::OnAccess( const fs::path& path, const NotifyEvent& event )ι->void{
		Trace( _tags, "IDriveChange::OnAccess( {}, {} )", path.string(), event.ToString() );
		//Logging::Log( Logging::MessageBase(_logTag, "IDriveChange::OnAccess( {}, {} )", "__FILE__", "__func__", 201), path.string(), event );
	}
	α IDriveChange::OnModify( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnModify( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnAttribute( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnAttribute( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnCloseWrite( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnCloseWrite( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnCloseNoWrite( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnCloseNoWrite( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnOpen( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnOpen( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnMovedFrom( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnMovedFrom( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnMovedTo( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnMovedTo( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnCreate( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnCreate( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnDelete( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnDelete( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnDeleteSelf( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnDeleteSelf( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnMoveSelf( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnMoveSelf( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnUnmount( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnUnmount( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnQOverflow( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnQOverflow( {}, {} )", path.string(), event.ToString() ); }
	α IDriveChange::OnIgnored( const fs::path& path, const NotifyEvent& event )ι->void{ Trace( _tags, "IDriveChange::OnIgnored( {}, {} )", path.string(), event.ToString() ); }
}
