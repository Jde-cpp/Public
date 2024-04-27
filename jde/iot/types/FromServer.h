#pragma once
#include "../uatypes/Node.h"

#define $ ι->MessageUnion
#define var const auto
#define SET(x) MessageUnion m; m.x( p.release() ); return m
#define NEW(T) auto p = mu<T>()
#define NEWR(T) NEW(T); p->set_request_id( id )
namespace Jde::Iot::FromServer{
	namespace Common=Jde::Web::FromServer;
	Ξ ToAck( uint32 id )${ NEW(Common::Acknowledgement); p->set_id( id ); SET(set_allocated_acknowledgement); }
	Ξ ToException( uint32 id, string&& x )${ NEWR(Common::Exception); p->set_message( move(x) ); SET(set_allocated_exception); }
	Ξ ToUnsubscribeResult( uint32 id, flat_set<NodeId>&& successes, flat_set<NodeId>&& failures )${
		NEWR(UnsubscribeResult);
		for_each( move(successes), [&p](var& n){*p->add_successes() = n.ToProto();} );
		for_each( move(failures), [&p](var& n){*p->add_failures() = n.ToProto();} );
		SET(set_allocated_unsubscribe_result);
	}
}
#undef $
#undef var
#undef SET
#undef NEW
#undef NEWR
