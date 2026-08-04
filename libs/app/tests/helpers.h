#pragma once
//Builders shared by the suites:  log entries, the two archive wire shapes ArchiveFile is fed, and schema-free TableQLs.
#include <jde/fwk/log/Entry.h>
#include <jde/app/proto/Log.pb.h>
#include <jde/app/proto/LogProto.h>
#include <jde/ql/types/Parser.h>
#include <jde/ql/types/TableQL.h>

#define let const auto

namespace Jde::App::Tests{
	Ξ tp( uint seconds )ι->TimePoint{ return Clock::from_time_t( 1'700'000'000 )+std::chrono::seconds{seconds}; }

	Ξ entry( TimePoint time, ELogLevel level, uint32_t line, string text, vector<string> args={}, string file="src/x.cpp", string function="Fn", ELogTags tags=ELogTags::Test, UserPK user=UserPK{7} )ι->Logging::Entry{
		return Logging::Entry{ level, tags, line, time, user, move(text), move(file), move(function), move(args) };
	}

	//The strings an entry references, in the archive file's per-kind tables.
	Ξ addStrings( Log::Proto::ArchiveFile& af, const Logging::Entry& e )ι->void{
		*af.add_templates() = LogProto::ToString( e.Id(), string{e.Text} );
		*af.add_files() = LogProto::ToString( e.FileId(), string{e.File()} );
		*af.add_functions() = LogProto::ToString( e.FunctionId(), string{e.Function()} );
		for( let& arg : e.Arguments )
			*af.add_args() = LogProto::ToString( Logging::Entry::GenerateId(arg), string{arg} );
	}

	//What ArchiveAwait reads back out of an archived day:  entries plus the whole string table.
	Ξ archiveProto( const vector<Logging::Entry>& entries )ι->Log::Proto::ArchiveFile{
		Log::Proto::ArchiveFile af;
		for( let& e : entries ){
			*af.add_entries() = LogProto::LogEntryFile( e );
			addStrings( af, e );
		}
		return af;
	}

	//What the daily file/ProtoLog buffer holds:  a flat FileEntry stream, each string once, ahead of the entry naming it.
	Ξ fileEntries( const vector<Logging::Entry>& entries )ι->vector<Log::Proto::FileEntry>{
		vector<Log::Proto::FileEntry> y;
		auto addString = [&]( uuid id, string value ){
			Log::Proto::FileEntry fe;
			*fe.mutable_str() = LogProto::ToString( id, move(value) );
			y.emplace_back( move(fe) );
		};
		for( let& e : entries ){
			addString( e.Id(), string{e.Text} );
			addString( e.FileId(), string{e.File()} );
			addString( e.FunctionId(), string{e.Function()} );
			for( let& arg : e.Arguments )
				addString( Logging::Entry::GenerateId(arg), arg );
			Log::Proto::FileEntry fe;
			*fe.mutable_entry() = LogProto::LogEntryFile( e );
			y.emplace_back( move(fe) );
		}
		return y;
	}

	//`system` leaves _dbTable null - which is what the log tables are:  they have no view, and nothing here opens a data source.
	Ξ table( sv jsonName, sv args="{}" )ε->QL::TableQL{
		static const vector<sp<DB::AppSchema>> noSchemas;
		return QL::TableQL{ string{jsonName}, QL::Parser::ParseArgs(string{args}), ms<jobject>(), noSchemas, true };
	}
	//TableQL::AddColumn resolves the db column off _dbTable, which is null here;  the archive reads JsonName only.
	Ξ addColumns( QL::TableQL& t, std::initializer_list<sv> columns )ι->QL::TableQL&{
		for( let& c : columns )
			t.Columns.push_back( QL::ColumnQL{string{c}, {}} );
		return t;
	}
	Ξ addTable( QL::TableQL& parent, sv jsonName, std::initializer_list<sv> columns, sv args="{}" )ε->QL::TableQL&{
		parent.Tables.emplace_back( table(jsonName, args) );
		return addColumns( parent.Tables.back(), columns );
	}
}
#undef let
