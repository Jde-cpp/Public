#pragma once

#define $ noexcept->Proto::Results::MessageUnion
#define _ Proto::Results::
#define var const auto
#define SET(x) Proto::Results::MessageUnion m; m.x( p ); return m
namespace Jde::Markets
{
	//using namespace Proto::Results;
	//using M=Proto::Results::MessageUnion;
	Ξ ToMessage( string what, ::OrderId id, int code )${ auto p = new Proto::Results::Error(); p->set_message( move(what) ); p->set_request_id( id ); p->set_code( code ); SET(set_allocated_error); }
	Ξ ToMessage( ::OrderId id, sp<const OrderStatus> pStatus, sp<const OrderState> pState )${ auto p = new Proto::Results::OrderUpdate(); if( pStatus ) p->set_allocated_status( pStatus->ToProto().release() ); else p->set_allocated_status( OrderStatus{ .Id=id }.ToProto().release() ); if( pState ) p->set_allocated_state( ToProto(*pState).release() ); SET( set_allocated_order_update ); }
	Ξ ToMessage( Proto::Results::OptionValues* p )${ SET(set_allocated_options); }
	Ξ ToMessage( Proto::Results::OpenOrder* p )${ SET(set_allocated_open_order); }
	Ξ ToRatioMessage( const std::map<string,double>& x, uint32 c )${ auto p = new Proto::Results::Fundamentals(); p->set_request_id( c ); for( var& [n,v] : x ) (*p->mutable_values())[n] = v; SET(set_allocated_fundamentals); }
}
#undef $
#undef SET
#undef _
#undef var