#include "../usings.h"

namespace Jde::App::LogProto{
	α LogEntryClient( Logging::Entry&& m )ι->Log::Proto::LogEntryClient;
	α LogEntryFile( const Logging::Entry& m )ι->Log::Proto::LogEntryFile;
	α LogEntryFile( const Logging::Entry& m, App::ProgramPK appPK, App::ProgInstPK instancePK )ι->Log::Proto::LogEntryFileExternal;
	α FromLogEntry( Log::Proto::LogEntryClient&& m )ι->Logging::Entry;
	α ToString( uuid id, string&& value )ι->Log::Proto::String;

	α DebugString( const Log::Proto::FileEntry& f )ι->string;
	α DebugString( const Log::Proto::LogEntryFile& f )ι->string;
	α DebugString( const Log::Proto::LogEntryFileExternal& f )ι->string;
}