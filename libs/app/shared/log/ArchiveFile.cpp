#include <jde/app/log/ArchiveFile.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/protobuf.h>

#define let const auto

namespace Jde::App{
	using App::Log::Proto::LogEntryFile;
	using Protobuf::ToGuid;
	Ω find( const auto& map, string uuid )ι->str{
		let it = map.find( ToGuid(uuid) );
		return it==map.end() ? Str::Empty() : it->second;
	}

	α ArchiveFile::Message( const LogEntryFile& entry )Ι->string{
		vector<string> args;
		for( let& argId : entry.args() )
			args.emplace_back( find(Args, argId) );
		let fmt = find( Templates, entry.template_id() );
		return Str::TryFormat( fmt, args );
	}

	α ArchiveFile::Test( const QL::Filter& filter, TimePoint time, const LogEntryFile& entry )Ι->bool{
		bool valid{ true };
		valid = valid && filter.Test( "time", time );
		valid = valid && filter.TestF<string>( "text", [&](){ return find(Templates, entry.template_id()); } );
		valid = valid && filter.Test( "level", (uint8)entry.level() );
		valid = valid && filter.Test( "tags", entry.tags() );
		valid = valid && filter.Test( "line", entry.line() );
		valid = valid && filter.TestF<uuid>( "templateId", [&](){ return ToGuid(entry.template_id()); } );
		valid = valid && filter.TestF<string>( "message", [&](){ return Message(entry); } );
		if( valid && filter.ColumnFilters.contains("args") ){
			for( let& argId : entry.args() ){
				if( valid = filter.TestF<string>("args", [&](){ return find(Args, argId); }); !valid )
					break;
			}
		}
		return valid;
	}
	α ArchiveFile::Test( const QL::TableQL& q, TimePoint time, const LogEntryFile& entry )Ε->bool{
		let& filter = q.Filter();
		bool valid = filter.Empty() || Test( filter, time, entry );

		if( valid ){
			auto testFileFunction = [&]( const auto& filter, const auto& map, string id )ι->bool {
				if( auto valid = filter.template TestF<string>( "id", [&](){return boost::uuids::to_string(ToGuid(id));} ); !valid )
					return false;
				if( auto valid = filter.template TestF<string>( "name", [&](){return find(map, id);} ); !valid )
					return false;
				return true;
			};
			for( auto& sub : q.Tables ){
				if( sub.JsonName=="user" ){
					if( valid = sub.Filter().Test( "id", entry.user_pk() ); !valid )
						break;
				}
				else if( valid = sub.JsonName=="file" ? testFileFunction(sub.Filter(), Files, entry.file_id()) : true; !valid )
					break;
				else if( valid = sub.JsonName=="function" ? testFileFunction(sub.Filter(), Functions, entry.function_id()) : true; !valid )
					break;
			}
		}
		return valid;
	}

	α ArchiveFile::Append( const QL::TableQL& q, App::Log::Proto::ArchiveFile&& af )ε->void{
		auto addStrings = []( auto&& collection, auto& map ){
			for( int i=0; i<collection.size(); ++i ){
				auto& s = collection.at(i);
				let id = ToGuid(s.id());
				map[id] = move(*s.mutable_value());
			}
		};
		addStrings( move(*af.mutable_templates()), Templates );
		addStrings( move(*af.mutable_files()), Files );
		addStrings( move(*af.mutable_functions()), Functions );
		addStrings( move(*af.mutable_args()), Args );
		for( int i=0; i<af.entries_size(); ++i ){
			auto entry = af.mutable_entries(i);
			let time = Protobuf::ToTimePoint( entry->time() );
			if( Test(q, time, *entry) )
				Entries[time].emplace_back( move(*entry) );
		}
	}
	using App::Log::Proto::FileEntry;
	α ArchiveFile::Append( const QL::Filter& filter, vector<FileEntry>&& entries )ε->void{
		vector<LogEntryFile> logEntries;
		flat_map<uuid, string> strings;
		for( auto& fe : entries ){
			if( fe.value_case()==FileEntry::ValueCase::kEntry )
				logEntries.emplace_back( move(*fe.mutable_entry()) );
			else if( fe.value_case()==FileEntry::ValueCase::kStr )
				strings[ToGuid(fe.str().id())] = move(*fe.mutable_str()->mutable_value());
		}
		auto addString = [&]( auto& map, auto& id ){
			map.emplace( ToGuid(id), move(strings[ToGuid(id)]) );
		};
		for( let& entry : logEntries ){
			let time = Protobuf::ToTimePoint( entry.time() );
			TRACET( ELogTags::Test, "entry time: {}", ToIsoString(time) );
			if( !Test(filter, time, entry) )
				continue;
			addString( Templates, entry.template_id() );
			addString( Files, entry.file_id() );
			addString( Functions, entry.function_id() );
			for( let& argId : entry.args() )
				addString( Args, argId );
			Entries[time].emplace_back( move(entry) );
		}
	}
	α ArchiveFile::ToJson( const QL::TableQL& ql )Ι->jarray{
		jarray results;
		for( let& [time, entries] : Entries ){
			for( let& entry : entries ){
				jobject o;
				for( let& col : ql.Columns ){
					let& name = col.JsonName;
					if( name=="template" )
						o[name] = find( Templates, entry.template_id() );
					else if( name=="args" ){
						jarray args;
						for( auto&& arg : entry.args() )
							args.push_back( {find(Args, arg)} );
						o[name] = move(args);
					}
					else if( name=="level" )
						o[name] = ToString( (ELogLevel)entry.level() );
					else if( name=="tags" )
						o[name] = ToArray( (ELogTags)entry.tags() );
					else if( name=="line" )
						o[name] = entry.line();
					else if( name=="time" )
						o[name] = ToIsoString( Protobuf::ToTimePoint(entry.time()) );
					else if( name=="message" )
						o[name] = Message( entry );
					else if( name=="id" )
						o[name] = boost::uuids::to_string( ToGuid(entry.template_id()) );
				}
				for( auto&& table : ql.Tables ){
					if( table.JsonName=="file" ){
						jobject o;
						if( table.FindColumn("name") )
							o["name"] = find( Files, entry.file_id() );
						if( table.FindColumn("id") )
							o["id"] = boost::uuids::to_string( ToGuid(entry.file_id()) );
						o[table.JsonName] = move(o);
					}
					else if( table.JsonName=="function" ){
						jobject o;
						if( table.FindColumn("name") )
							o["name"] = find( Functions, entry.function_id() );
						if( table.FindColumn("id") )
							o["id"] = boost::uuids::to_string( ToGuid(entry.function_id()) );
						o[table.JsonName] = move(o);
					}
					else if( table.JsonName=="user" ){
						jobject o;
						if( table.FindColumn("id") )
							o["id"] = entry.user_pk();
						o[table.JsonName] = move(o);
					}
				}
				results.push_back( move(o) );
			}
		}
		return results;
	}
}