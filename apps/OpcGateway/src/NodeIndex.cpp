#include "NodeIndex.h"
#include <jde/fwk/process/execution.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/str.h>
#include "UAClient.h"

#define let const auto
namespace Jde::Opc::Gateway{
	α NodeIndex::ReadyAwait::await_ready()ι->bool{
		sl _{ _index._mutex };
		return _index._state==EState::Ready && !_refresh;
	}
	α NodeIndex::ReadyAwait::Suspend()ι->void{
		_index.StartLocked( move(_client), _refresh, this );
	}

	α NodeIndex::StartLocked( sp<UAClient>&& client, bool refresh, AnyVoidAwait* waiter )ι->void{
		bool launch{}, ready{};
		{
			ul _{ _mutex };
			if( _state==EState::Crawling )
				{}//join the in-flight crawl - a refresh included:  its result is fresh enough.
			else if( _state!=EState::Ready || refresh ){
				_state = EState::Crawling;
				launch = true;
			}
			else
				ready = true;
			if( waiter && !ready )
				_waiters.push_back( waiter );
		}
		if( launch )
			Crawl( move(client) );//fire & forget:  the task keeps the client (and so this index) alive until Finish.
		if( waiter && ready )
			Post( [waiter]{ waiter->Resume(); } );//a Finish raced in between await_ready and here - never resume inside await_suspend.
	}

	α NodeIndex::Crawl( sp<UAClient> client )ι->TAwait<Browse::Response>::Task{
		vector<Entry> entries;
		bool truncated{};
		up<Exception> error;
		let start = std::chrono::steady_clock::now();
		//value_or defaults mirror config/Opc.Gateway.jsonnet - keep them in lockstep.
		let maxDepth = Settings::FindNumber<uint8>( "/gateway/search/maxDepth" ).value_or( 12 );
		let maxNodes = Settings::FindNumber<uint>( "/gateway/search/maxNodes" ).value_or( 25000 );
		let includeServer = Settings::FindBool( "/gateway/search/includeServer" ).value_or( false );
		try{
			let defaultNs = client->DefaultBrowseNs();
			struct Pending{ NodeId Id; string Path; uint8 Depth; };
			std::deque<Pending> pending;
			pending.push_back( {NodeId::ObjectsFolder(), {}, 0} );
			flat_set<NodeId> seen; seen.emplace( NodeId::ObjectsFolder() );//cycles:  hierarchical references may still reach a node twice.
			bool warnedSlash{};
			constexpr UA_BrowseResultMask mask{ UA_BROWSERESULTMASK_BROWSENAME|UA_BROWSERESULTMASK_DISPLAYNAME|UA_BROWSERESULTMASK_NODECLASS };
			while( pending.size() && !truncated ){
				auto parent = move( pending.front() ); pending.pop_front();
				auto response = co_await Browse::FoldersAwait{ Browse::Request::Hierarchical(NodeId{parent.Id}, mask), client };
				if( response.resultsSize==0 )
					continue;
				if( response.results[0].continuationPoint.length ){//v1 does not BrowseNext:  the folder is indexed up to the server's per-browse cap.
					WARNT( BrowseTag, "[{}]search index: '{}' returned a continuation point - children beyond the server's limit are not indexed.", hex(client->Handle()), parent.Path );
					truncated = true;
				}
				response.VisitWhile( 0, [&]( const UA_ReferenceDescription& ref ){
					NodeId id{ ref.nodeId.nodeId };
					if( seen.find(id)!=seen.end() )
						return true;
					if( !includeServer && ref.nodeId.nodeId.namespaceIndex==0 && ref.nodeId.nodeId.identifierType==UA_NODEIDTYPE_NUMERIC && ref.nodeId.nodeId.identifier.numeric==UA_NS0ID_SERVER )
						return true;//the Server object's diagnostics/capabilities subtree is hundreds of nodes nobody searches for.
					string browse{ ToSV(ref.browseName.name) };
					if( browse.find('/')!=string::npos ){//unroutable:  the node url is '/'-delimited browse segments.
						if( !warnedSlash ){
							WARNT( BrowseTag, "[{}]search index: skipping browse names containing '/' ('{}' under '{}').", hex(client->Handle()), browse, parent.Path );
							warnedSlash = true;
						}
						return true;
					}
					let segment = ref.browseName.namespaceIndex==defaultNs ? browse : Ƒ( "{}~{}", ref.browseName.namespaceIndex, browse );
					string path = parent.Path.empty() ? segment : Ƒ( "{}/{}", parent.Path, segment );
					let depth = (uint8)( parent.Depth+1 );
					if( (ref.nodeClass==UA_NODECLASS_OBJECT || ref.nodeClass==UA_NODECLASS_VARIABLE) && depth<maxDepth )//DI/IA hang children under variables too.
						pending.push_back( {NodeId{id}, path, depth} );
					seen.emplace( NodeId{id} );
					Entry e{ move(id), move(path), string{ToSV(ref.displayName.text)}, browse, {}, Str::ToLower(browse), ref.browseName.namespaceIndex, ref.nodeClass, depth };
					e.NameLower = Str::ToLower( e.Name );
					entries.push_back( move(e) );
					if( entries.size()>=maxNodes ){
						WARNT( BrowseTag, "[{}]search index: stopped at /gateway/search/maxNodes={}.", hex(client->Handle()), maxNodes );
						truncated = true;
					}
					return !truncated;
				} );
			}
			let ms = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now()-start ).count();
			INFOT( BrowseTag, "[{}]Indexed {} nodes under Objects for '{}' (maxDepth={}) in {}ms{}.", hex(client->Handle()), entries.size(), client->Target(), maxDepth, ms, truncated ? " - truncated" : "" );
		}
		catch( runtime_error& e ){
			WARNT( BrowseTag, "[{}]search index for '{}' failed: {}", hex(client->Handle()), client->Target(), e.what() );
			if( auto p = dynamic_cast<Exception*>(&e); p )
				error = p->Move();
			else
				error = mu<Exception>( move(e) );
		}
		Finish( move(entries), truncated, move(error) );
	}

	α NodeIndex::Finish( vector<Entry>&& entries, bool truncated, up<Exception> error )ι->void{
		vector<AnyVoidAwait*> waiters;
		{
			ul _{ _mutex };
			if( error )
				_state = EState::Failed;//the next search retries from scratch.
			else{
				_entries = move( entries );
				_truncated = truncated;
				_state = EState::Ready;
			}
			waiters = move( _waiters );
			_waiters.clear();
		}
		for( auto* waiter : waiters ){//outside the lock:  Resume may run the awaiter to completion (and back into Search) inline - it is the last use of the waiter.
			if( error )
				waiter->ResumeExp( Exception{*error} );
			else
				waiter->Resume();
		}
	}

	α NodeIndex::Search( sv text, uint limit )Ι->vector<Entry>{
		vector<const Entry*> hits;
		vector<Entry> y;
		sl _{ _mutex };
		vector<uint8> ranks; ranks.reserve( _entries.size() );
		for( let& e : _entries ){
			uint8 rank = e.NameLower.starts_with(text) ? 0 : e.BrowseLower.starts_with(text) ? 1 : e.NameLower.find(text)!=string::npos || e.BrowseLower.find(text)!=string::npos ? 2 : 3;
			ranks.push_back( rank );
			if( rank<3 )
				hits.push_back( &e );
		}
		auto rankOf = [&]( const Entry* e ){ return ranks[e-&_entries[0]]; };
		std::ranges::sort( hits, [&]( const Entry* a, const Entry* b ){
			let ra = rankOf(a), rb = rankOf(b);
			return ra!=rb ? ra<rb : a->Depth!=b->Depth ? a->Depth<b->Depth : a->NameLower<b->NameLower;
		} );
		if( limit && hits.size()>limit )
			hits.resize( limit );
		y.reserve( hits.size() );
		for( auto* hit : hits ){
			y.push_back( *hit );
			y.back().Rank = rankOf( hit );
		}
		return y;
	}
}
