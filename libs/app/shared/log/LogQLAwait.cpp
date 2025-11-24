#include <jde/app/log/LogQLAwait.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/chrono.h>
#include "LogAwait.h"

#define let const auto

namespace Jde::App{
	α toJson( Logging::Entry&& entry, const QL::TableQL& ql )ι->jobject{
		jobject jEntry;
		for( let& col : ql.Columns ){
			let& name = col.JsonName;
			if( name=="text" )
				jEntry[name] = move(entry.Text);
			else if( name=="args" ){
				jarray args;
				for( auto&& arg : entry.Arguments )
					args.push_back( jstring{move(arg)} );
				jEntry[name] = move(args);
			}
			else if( name=="level" )
				jEntry[name] = ToString( (ELogLevel)entry.Level );
			else if( name=="tags" )
				jEntry[name] = ToArray( entry.Tags );
			else if( name=="line" )
				jEntry[name] = entry.Line;
			else if( name=="time" )
				jEntry[name] = ToIsoString( entry.Time );
			else if( name=="message" )
				jEntry[name] = entry.Message();
			else if( name=="id" )
				jEntry[name] = boost::uuids::to_string( entry.Id() );
			else if( name=="userId" )
				jEntry[name] = entry.UserPK.Value;
		}
		for( auto&& table : ql.Tables ){
			if( table.JsonName=="file" ){
				jobject o;
				if( table.FindColumn("name") )
					o["name"] = entry.File();;
				if( table.FindColumn("id") )
					o["id"] = boost::uuids::to_string( entry.FileId() );
				jEntry[table.JsonName] = move(o);
			}
			else if( table.JsonName=="function" ){
				jobject o;
				if( table.FindColumn("name") )
					o["name"] = entry.Function();;
				if( table.FindColumn("id") )
					o["id"] = boost::uuids::to_string( entry.FunctionId() );
				jEntry[table.JsonName] = move(o);
			}
			else if( table.JsonName=="user" ){
				jobject o;
				if( table.FindColumn("id") )
					o["id"] = entry.UserPK.Value;
				jEntry[table.JsonName] = move(o);
			}
		}
		return jEntry;
	}

	α LogQLAwait::Execute()ι->TAwait<ArchiveFile>::Task{
		try{
			auto archive = co_await LogAwait{ _ql, _sl };
			Resume( archive.ToJson(_ql) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}