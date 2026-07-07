#include <jde/app/log/ArchiveFile.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/app/proto/LogProto.h>

#define let const auto

namespace Jde::App{
	using App::Log::Proto::LogEntryFile;
	using Protobuf::ToGuid;

	ArchiveFile::ArchiveFile( const QL::Filter& q, vector<App::Log::Proto::FileEntry>&& entries )ε{
		Append( q, move(entries) );
	}

	α ArchiveFile::EntrySize()Ι->uint{
		uint size{};
		for( auto& [_,entries] : Entries )
			size += ( uint )entries.size();
		return size;
	}

	α ArchiveFile::IsComplete( const QL::Input& input )Ι->bool{
		let limit = input.Limit();
		if( !limit )
			return false;
		let& orderBy = input.OrderByJson();
		if( orderBy.empty() || orderBy.begin()->first!="time" )
			return false;
		return EntrySize()>=limit+input.Offset();
	}
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
		valid = valid && filter.TestF<string>( "text", [&](){return find(Templates, entry.template_id());} );
		valid = valid && filter.Test( "level", (uint8)entry.level() );
		valid = valid && filter.TestOr( "tags", entry.tags() );
		valid = valid && filter.Test( "line", entry.line() );
		valid = valid && filter.TestF<uuid>( "templateId", [&](){return ToGuid(entry.template_id());} );
		valid = valid && filter.TestF<string>( "message", [&](){return Message(entry);} );
		if( valid && filter.ColumnFilters.contains("args") ){
			for( let& argId : entry.args() ){
				if( valid = filter.TestF<string>("args", [&](){return find(Args, argId);}); !valid )
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
				if( auto valid = filter.template TestF<string>("id", [&](){return to_string(ToGuid(id));}); !valid )
					return false;
				if( auto valid = filter.template TestF<string>("name", [&](){return find(map, id);}); !valid )
					return false;
				return true;
			};
			for( auto& sub : q.Tables ){
				if( sub.JsonName=="user" ){
					if( valid = sub.Filter().Test("id", entry.user_pk()); !valid )
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
				auto& s = collection.at( i );
				let id = ToGuid( s.id() );
				ASSERT_DESC( s.value().size() || id==EmptyStringMd5, "String with empty value must have empty md5." );
				map[id] = move( *s.mutable_value() );
			}
		};
		addStrings( move(*af.mutable_templates()), Templates );
		addStrings( move(*af.mutable_files()), Files );
		addStrings( move(*af.mutable_functions()), Functions );
		addStrings( move(*af.mutable_args()), Args );
		for( int i=0; i<af.entries_size(); ++i ){
			auto entry = af.mutable_entries( i );
			let time = Protobuf::ToTimePoint( entry->time() );
			if( Test(q, time, *entry) )
				Entries[time].emplace_back( move(*entry) );
		}
		for( int i=0; i<af.externalentries_size(); ++i ){
			auto entry = LogProto::ToEntry( move(*af.mutable_externalentries(i)) );
			let time = Protobuf::ToTimePoint( entry.time() );
			if( Test(q, time, entry) )
				Entries[time].emplace_back( move(entry) );
		}
	}
	using App::Log::Proto::FileEntry;
	α ArchiveFile::Append( const QL::Filter& filter, vector<FileEntry>&& entries )ε->void{
		vector<LogEntryFile> logEntries;
		flat_map<uuid, string> strings;
		for( auto& fe : entries ){
			if( fe.value_case()==FileEntry::ValueCase::kEntry )
				logEntries.emplace_back( move(*fe.mutable_entry()) );
			else if( fe.value_case()==FileEntry::ValueCase::kExternalEntry )
				logEntries.emplace_back( LogProto::ToEntry(move(*fe.mutable_external_entry())) );
			else if( fe.value_case()==FileEntry::ValueCase::kStr ){
				let id = ToGuid( fe.str().id() );
				auto& value = *fe.mutable_str()->mutable_value();
				ASSERT_DESC( value.size() || id==EmptyStringMd5, "String with empty value must have empty md5." );
				strings[id] = move( value );
			}
		}
		auto addString = [&]( auto& map, auto& id ){
			map.try_emplace( ToGuid(id), move(strings[ToGuid(id)]) );
		};
		for( let& entry : logEntries ){
			let time = Protobuf::ToTimePoint( entry.time() );
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

	α ArchiveFile::Sort( const vector<std::pair<string,bool>>& orderBy )Ι->vector<App::Log::Proto::LogEntryFile>{
		vector<App::Log::Proto::LogEntryFile> y;
		for( let& [ts,entries] : Entries )
			y.insert( y.end(), entries.begin(), entries.end() );

		if( orderBy.empty() || (orderBy[0].first=="time" && orderBy[0].second) )
			return y;

		std::sort( y.begin(), y.end(), [&](let& a, let& b){
			optional<bool> lessThan;
			for( let& [field,asc] : orderBy ){
				if( field=="time" ){
					let aTime = Protobuf::ToTimePoint( a.time() );
					let bTime = Protobuf::ToTimePoint( b.time() );
					if( aTime != bTime )
						lessThan = aTime<bTime;
				}
				else if( field=="file" )
					lessThan = a.file_id()==b.file_id() ? nullopt : optional<bool>{ a.file_id()<b.file_id() };
				else if( field=="function" )
					lessThan = a.function_id()==b.function_id() ? nullopt : optional<bool>{ a.function_id()<b.function_id() };
				else if( field=="level" )
					lessThan = a.level()==b.level() ? nullopt : optional<bool>{ a.level()<b.level() };
				else if( field=="line" )
					lessThan = a.line()==b.line() ? nullopt : optional<bool>{ a.line()<b.line() };
				else if( field=="message" ){
					let aMsg = Message( a );
					let bMsg = Message( b );
					lessThan = aMsg==bMsg ? nullopt : optional<bool>{ aMsg<bMsg };
				}
				else if( field=="tags" )
					lessThan = a.tags()==b.tags() ? nullopt : optional<bool>{ a.tags()<b.tags() };
				else if( field=="user" )
					lessThan = a.user_pk()==b.user_pk() ? nullopt : optional<bool>{ a.user_pk()<b.user_pk() };
				if( lessThan )
					return asc ? *lessThan : !*lessThan;
			}
			return false;
		} );
		return y;
	}
	α ArchiveFile::ToEntry( const QL::TableQL& table, const App::Log::Proto::LogEntryFile& entry, optional<flat_map<uuid,string>>& strings )Ι->jobject{
		jobject o;
		for( let& col : table.Columns ){
			let& name = col.JsonName;
			if( name=="templateId" ){
				let id = ToGuid( entry.template_id() );
				o[name] = to_string( id );
				if( strings )
					( *strings )[id] = find( Templates, entry.template_id() );
			}
			else if( name=="argIds" ){
				jarray args;
				for( auto&& arg : entry.args() ){
					let id = ToGuid( arg );
					args.push_back( {to_string(id)} );
					if( strings )
						( *strings )[id] = find( Args, arg );
				}
				o[name] = move( args );
			}
			else if( name=="level" )
				o[name] = Jde::ToString( (ELogLevel)entry.level() );
			else if( name=="tags" )
				o[name] = ToArray( (ELogTags)entry.tags() );
			else if( name=="line" )
				o[name] = entry.line();
			else if( name=="time" )
				o[name] = ToIsoString( Protobuf::ToTimePoint(entry.time()) )+'Z';
			else if( name=="userId" )
				o[name] = entry.user_pk();
			else if( name=="fileId" ){
				let id = ToGuid( entry.file_id() );
				o[name] = to_string( id );
				if( strings )
					( *strings )[id] = find( Files, entry.file_id() );
			}
			else if( name=="functionId" ){
				let id = ToGuid( entry.function_id() );
				o[name] = to_string( id );
				if( strings )
					( *strings )[id] = find( Functions, entry.function_id() );
			}
		}
		return o;
	}
	//logs( limit: $limit, offset: $offset, orderBy: $orderBy ){ entries{templateId argIds level tags line time userId fileId functionId} strings{id value} }
	α ArchiveFile::ToJson( const QL::TableQL& ql )Ι->jobject{
		let entries = Sort( ql.OrderByJson() );
		jobject o;
		jarray jentries;
		auto strings = ql.FindTable( "strings" ) ? flat_map<uuid,string>{} : optional<flat_map<uuid,string>>{};
		let entriesTable = ql.FindTable( "entries" );
		for( uint i=0; i<entries.size(); ++i ){
			if( i<ql.Offset() || (ql.Limit() && i>=ql.Offset()+ql.Limit()) )
				continue;
			auto& entry = entries.at( i );
			jobject jentry = entriesTable ? ToEntry( *entriesTable, entry, strings ) : jobject{};
			for( auto&& table : ql.Tables ){
				if( table.JsonName=="file" ){
					jobject jt;
					if( table.FindColumn("name") )
						jt["name"] = find( Files, entry.file_id() );
					if( table.FindColumn("id") )
						jt["id"] = to_string( ToGuid(entry.file_id()) );
					jentry[table.JsonName] = move( jt );
				}
				else if( table.JsonName=="function" ){
					jobject jt;
					if( table.FindColumn("name") )
						jt["name"] = find( Functions, entry.function_id() );
					if( table.FindColumn("id") )
						jt["id"] = to_string( ToGuid(entry.function_id()) );
					jentry[table.JsonName] = move( jt );
				}
				else if( table.JsonName=="user" ){
					jobject jt;
					if( table.FindColumn("id") )
						jt["id"] = entry.user_pk();
					jentry[table.JsonName] = move( jt );
				}
			}
			jentries.push_back( move(jentry) );
		}
		if( entriesTable )
			o["entries"] = move( jentries );
		if( strings ){
			jarray jstrings;
			for( let& [id,value] : *strings ){
				jobject s;
				s["id"] = to_string( id );
				s["value"] = value;
				jstrings.push_back( move(s) );
			}
			o["strings"] = move( jstrings );
		}
		return o;
	}
}