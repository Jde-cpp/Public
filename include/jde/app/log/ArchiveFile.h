#pragma once
#include <jde/ql/types/TableQL.h>
#include <jde/app/shared/proto/Log.pb.h>

namespace Jde::App{
	struct ArchiveFile{
		ArchiveFile()ι=default;
		ArchiveFile( ArchiveFile&& x )ι=default;
		α operator=( ArchiveFile&& x )ι->ArchiveFile& = default;
		α Append( const QL::Filter& q, vector<App::Log::Proto::FileEntry>&& entries )ε->void;
		α Append( const QL::TableQL& q, App::Log::Proto::ArchiveFile&& af )ε->void;
		α ToJson( const QL::TableQL& ql )Ι->jarray;
		//QL::TableQL Query;
		std::map<TimePoint,vector<App::Log::Proto::LogEntryFile>> Entries;
		flat_map<uuid,string> Templates;
		flat_map<uuid,string> Files;
		flat_map<uuid,string> Functions;
		std::map<uuid,string> Args;
	private:
		α Message( const App::Log::Proto::LogEntryFile& entry )Ι->string;
		α Test( const QL::TableQL& q, TimePoint time, const App::Log::Proto::LogEntryFile& entry )Ε->bool;
		α Test( const QL::Filter& filter, TimePoint time, const App::Log::Proto::LogEntryFile& entry )Ι->bool;
	};
}