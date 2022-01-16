#pragma once

#define $ noexcept->Proto::Results::MessageUnion
#define _ Proto::Results::
#define var const auto
#define SET(x) Proto::Results::MessageUnion m; m.x( p ); return m
namespace Jde::Markets
{
	Ξ ToMessage( string what, ::OrderId id, int code )${ auto p = new Proto::Results::Error(); p->set_message( move(what) ); p->set_request_id( id ); p->set_code( code ); SET(set_allocated_error); }
	Ξ ToMessage( ::OrderId id, sp<const OrderStatus> pStatus, sp<const OrderState> pState )${ auto p = new Proto::Results::OrderUpdate(); if( pStatus ) p->set_allocated_status( pStatus->ToProto().release() ); else p->set_allocated_status( OrderStatus{ .Id=id }.ToProto().release() ); if( pState ) p->set_allocated_state( ToProto(*pState).release() ); SET( set_allocated_order_update ); }
	Ξ ToMessage( Proto::Results::OptionValues* p )${ SET(set_allocated_options); }
	Ξ ToMessage( Proto::Results::OpenOrder* p )${ SET(set_allocated_open_order); }
	Ξ ToMessage( Proto::Results::EResults t, int v )${ auto p = new Proto::Results::MessageValue(); p->set_type( t ); p->set_int_value( v ); SET(set_allocated_message); }
	Ξ ToMessage( Proto::Results::EResults t, string v )${ auto p = new Proto::Results::MessageValue(); p->set_type( t ); p->set_string_value( move(v) ); SET(set_allocated_message); }
	Ξ ToMessage( Proto::Results::EResults t, uint32 client, ContractPK c, double v )${ auto p = new Proto::Results::ContractValue(); p->set_type( t ); p->set_request_id( client ); p->set_value( v ); p->set_contract_id( c ); SET(set_allocated_contract_value); }
	Ξ ToRatioMessage( const std::map<string,double>& x, uint32 c )${ auto p = new Proto::Results::Fundamentals(); p->set_request_id( c ); for( var& [n,v] : x ) (*p->mutable_values())[n] = v; SET(set_allocated_fundamentals); }
}
#undef $
#undef SET
#undef _
#undef var