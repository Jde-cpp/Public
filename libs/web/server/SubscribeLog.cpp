#include <jde/web/server/SubscribeLog.h>
#include <jde/ql/types/Subscription.h>
#include <jde/web/server/IWebsocketSession.h>

#define let const auto

namespace Jde::Web::Server{
	struct LogSubscription final : QL::Subscription{
		LogSubscription( LogSubscription&& sub )ι=default;
		LogSubscription( QL::Subscription&& sub )ε:
			QL::Subscription{ move(sub) }{
			let& filter = Fields.Filter();
			if( let kv = filter.ColumnFilters.find("level"); kv != filter.ColumnFilters.end() && kv->second.size()==1 ){
				let& filterValue = kv->second.front();
				auto level = filterValue.Value.is_string() ? ToLogLevel( filterValue.Value.get_string() ) : ELogLevel::Trace;
				if( filterValue.Operator==DB::EOperator::Greater )
					level = ( ELogLevel )( underlying(level)+1 );
				MinLevel = level;
			}
			if( let kv = filter.ColumnFilters.find("tags"); kv != filter.ColumnFilters.end()  && kv->second.size()==1 )
				Tags = ToLogTags( kv->second.front().Value );
		}
		ELogLevel MinLevel{ ELogLevel::Trace };
		ELogTags Tags{ ELogTags::All };
	};
	struct SessionSubscription{
		SessionSubscription( sp<IWebsocketSession> session, QL::Subscription&& sub )ι:Session{ move(session) },Sub{ move(sub) }{}
		sp<IWebsocketSession> Session;
		LogSubscription Sub;
	};

	vector<SessionSubscription> _subs; shared_mutex _mutex;
	α SubscribeLog::Shutdown( bool /*terminate*/ )ι->void{
		ul _{ _mutex };
		_subs.clear();
	}

	α SubscribeLog::Add( sp<Web::Server::IWebsocketSession> session, vector<QL::Subscription>&& subs )ι->void{
		ul _{ _mutex };
		for( auto& request : subs ){
			SessionSubscription s{ session, move(request) };
			if( s.Sub.MinLevel<MinLevel(s.Sub.Tags) )
				SetLevel( s.Sub.Tags, s.Sub.MinLevel );
			_subs.push_back( move(s) );
		}
	}
	α SubscribeLog::Write( const Logging::Entry& m )ι->void{
		Write( m, _appPK, _instancePK );
	}
	α SubscribeLog::Write( const Logging::Entry& m, uint32 appPK, uint32 instancePK )ι->void{
		sl _{ _mutex };
		for( let& s : _subs ){
			bool valid{ m.Level>=s.Sub.MinLevel };
			valid = valid && !empty( m.Tags & s.Sub.Tags );
			if( !valid )
				continue;
			auto& filter = s.Sub.Fields.Filter();
			valid = valid && filter.Test( "time", m.Time );
			valid = valid && filter.Test( "text", m.Text );
			valid = valid && filter.Test( "line", m.Line );
			valid = valid && filter.Test( "templateId", m.Id() );
			valid = valid && filter.Test( "message", m.Message() );
			valid = valid && filter.Test( "appPK", appPK );
			valid = valid && filter.Test( "appInstancePK", instancePK );
			if( valid && filter.ColumnFilters.contains("args") ){
				for( let& arg : m.Arguments ){
					if( valid = filter.Test("args", arg); !valid )
						break;
				}
			}
			if( valid )
				s.Session->WriteSubscription( _appPK, _instancePK, m, s.Sub );
		}
	}
}