#pragma once
#define $ noexcept->M
#define SET(x) Proto::Results::MessageUnion m; m.x( p ); return m
namespace Jde::Markets
{
	using M=Proto::Results::MessageUnion;
	Ξ ToMessage( string what, ::OrderId id, int code )${ auto p = mu<Proto::Results::Error>(); p->set_message( move(what) ); p->set_request_id( id ); p->set_code( code ); M m; m.set_allocated_error( p.release() ); return m; }
	Ξ ToMessage( ::OrderId id, sp<const OrderStatus> pStatus, sp<const OrderState> pState )${ auto p = mu<Proto::Results::OrderUpdate>(); if( pStatus ) p->set_allocated_status( pStatus->ToProto().release() ); else p->set_allocated_status( OrderStatus{ .Id=id }.ToProto().release() ); if( pState ) p->set_allocated_state( ToProto(*pState).release() ); Proto::Results::MessageUnion m; m.set_allocated_order_update( p.release() ); return m; }
	Ξ ToMessage( Proto::Results::OptionValues* p )${ SET(set_allocated_options); }
	Ξ ToMessage( Proto::Results::OpenOrder* p )${ SET(set_allocated_open_order); }
}
#undef $