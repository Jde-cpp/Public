#include <jde/app/log/ProtoLog.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/co/Timer.h>
#include <jde/fwk/io/proto.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/app/shared/proto/App.FromClient.h>

namespace Jde::App{
	#define let const auto

	ProtoLog::ProtoLog( const jobject& settings )ι:
		Logging::ILogger{ settings },
		_delay{ Json::FindDuration(settings, "delay", ELogLevel::Error).value_or(1min) },
		_path{ Json::FindString(settings, "path").value_or(Process::ApplicationDataFolder()/"logs") },
		_tz{ Json::FindTimeZone(settings, "timeZone", *std::chrono::current_zone()) }{
			Executor();//locks up if starts in StartTimer.
			Execution::Run();
			Process::AddShutdownFunction( [this]( bool /*terminate*/ ){
				_delay = 0s;
				ResetTimer();
			});
	}
	α ProtoLog::Shutdown( bool terminate )ι->void{
		if( !terminate && !_toSave.empty() ){
			try{
				IO::SaveBinary<byte>( _path/"log.binpb", _toSave );
			}
			catch( exception& e )
			{}
		}
	}

	α ProtoLog::Write( const Logging::Entry& e )ι->void{
		if( !empty(e.Tags & _tags) )//recursion guard
			return;
		auto proto = FromClient::LogEntryFile( e );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_entry() = move(proto);
		auto data = Jde::Proto::SizePrefixed( fileEntry );
		lg _{_mutex};
		AddString( e.Id(), e.Text );
		AddString( e.FileId(), e.File() );
		AddString( e.FunctionId(), e.Function() );
		AddArguments( e.Arguments, fileEntry.entry().args() );
		std::copy( data.begin(), data.end(), std::back_inserter(_toSave) );
		if( _toSave.size()>=_delaySize )
			Save();
		else if( !_timer )
			StartTimer();
	}
	α ProtoLog::Save()ι->VoidAwait::Task{
		auto toSave = move(_toSave);
		ResetTimer();
		_toSave.reserve( toSave.size() );
		if( _strings.size()>1024 )
			_strings.resize( 1024 );
		if( _args.size()>1024 )
			_args.resize( 1024 );
		try{
			co_await IO::WriteAwait( _path/"log.binpb", move(toSave), true, _tags );
		}
		catch( exception& e )
		{}
	}
	α ProtoLog::Load()ι->vector<App::Log::Proto::FileEntry>{
		vector<App::Log::Proto::FileEntry> y;
		auto content = BlockAwait<TAwait<string>,string>( IO::ReadAwait(_path/"log.binpb") );
		for( auto p = content.data(), end = content.data() + content.size(); p+4<end; ){
			//uint32 test = *(uint32*)p;
			uint32 length{};
			for( auto i=3; i>=0; --i ){
				const byte b = (byte)*p++;
				length = (length<<8) | (uint32)b;
			}
			std::cout << "iLength: " << length << std::endl;
			//ASSERT( test==length );
			if( p+length<content.data()+content.size() )
				y.push_back( Jde::Proto::Deserialize<App::Log::Proto::FileEntry>((google::protobuf::uint8*)p, (int)length) );
			p += length;
		}
		return y;
	}

	α ProtoLog::AddString( uuid id, sv str )ι->void{
		AddString( id, str, _strings );
	}
	α ProtoLog::AddString( uuid id, sv str, std::deque<uuid>& cache )ι->void{
		if( let i = find( cache, id ); i!=cache.end() )
			return;//TODO update position
		cache.push_front( id );
		App::Log::Proto::FileEntry fileEntry;
		*fileEntry.mutable_str() = FromClient::ToString(id, string{str});
		auto data = Jde::Proto::SizePrefixed( fileEntry );
		std::copy( data.begin(), data.end(), std::back_inserter(_toSave) );//TODO copy in SizePrefixed
	}
	α ProtoLog::AddArguments( const vector<string>& args, ::google::protobuf::RepeatedPtrField<std::string> ids )ι->void{
		ASSERT( args.size()==(uint)ids.size() );
		for( uint i=0; i<args.size(); ++i )
			AddString( Jde::Proto::ToGuid(ids.Get(i)), args[i], _args );
	}
	α ProtoLog::StartTimer()->VoidAwait::Task{
		if( _delay==0s )
			co_return;
		_timer = mu<DurationTimer>( _delay, _tags, SRCE_CUR );
		try{
			co_await *_timer;
			lg _{_mutex};
			if( !_toSave.empty() )
				Save();
		}
		catch( const IException& ){
			lg _{_mutex};
			if( _toSave.size() )
				StartTimer();
			else
				_timer = nullptr;
		}
	}

	α ProtoLog::ResetTimer()->void{
		_timer->Cancel();
	}
}