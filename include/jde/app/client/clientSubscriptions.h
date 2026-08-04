#pragma once
#include <jde/ql/usings.h>

namespace Jde::QL{ struct Subscription; }
namespace Jde::App::Proto::FromServer{ class Traces; }
namespace Jde::App::Client{ struct IAppClient; }
namespace Jde::App::Client::Subscriptions{
	//What a client asked for, as opposed to what the current socket happens to be carrying.  Server-side subscriptions die
	//with the socket, so Clear() correctly drops the live ids - but the *request* has to outlive them or nothing can be put
	//back afterwards.  Kept per (query, variables, listener) rather than per subscription id, because one query can ack
	//into several ids and replaying it re-creates all of them.
	struct Request final{
		string Query;
		jobject Variables;
		sp<QL::IListener> Listener;
		flat_set<QL::SubscriptionId> Ids;//what this request acked into, so Forget can drop it by id.
	};
	α Clear()ι->void;//live ids only - Remembered requests survive, that is the point.
	α Remember( string query, jobject variables, sp<QL::IListener> listener, const flat_set<QL::SubscriptionId>& ids )ι->void;
	α Forget( const sp<QL::IListener>& listener, const flat_set<QL::SubscriptionId>& ids )ι->void;//ids empty = every request of listener's.
	α Remembered()ι->vector<Request>;
	α StopListenRemote( sp<QL::IListener> listener, vector<QL::SubscriptionId> ids )ι->flat_set<QL::SubscriptionId>;
	α ListenRemote( sp<QL::IListener> listener, QL::Subscription&& sub )ι->void;
	α OnTraces( App::Proto::FromServer::Traces&& traces, QL::SubscriptionId requestId )ι->void;
	α OnWebsocketReceive( const jobject& m, QL::SubscriptionId clientId )ι->void;
	//Re-issues every Remembered request on a session that has just come up.  Returns the number replayed, 0 on a first
	//connect - which is what tells Client::Connect to run the initial subscribe instead.
	α Replay( sp<IAppClient> client )ι->uint;
}