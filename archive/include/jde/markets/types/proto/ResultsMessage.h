﻿#pragma once
//#pragma warning( disable : 4244 )
#include "results.pb.h"
//#pragma warning( default : 4244 )
#include "../MyOrder.h"

#define _ Proto::Results
#define $ noexcept->_::MessageUnion
#define let const auto
#define SET(x) _::MessageUnion m; m.x( p ); return m
namespace Jde::Markets
{
	using namespace Jde::Markets::Proto::Results;
	Ξ ToMessage( string what, ::OrderId id, int code )${ auto p = new _::Error(); p->set_message( move(what) ); p->set_request_id( id ); p->set_code( code ); SET(set_allocated_error); }
	Ξ ToMessage( ::OrderId id, sp<const OrderStatus> pStatus, sp<const ::OrderState> pState )${ auto p = new _::OrderUpdate(); if( pStatus ) p->set_allocated_status( pStatus->ToProto().release() ); else p->set_allocated_status( OrderStatus{ .Id=id }.ToProto().release() ); if( pState ) p->set_allocated_state( ToProto(*pState).release() ); SET( set_allocated_order_update ); }
	Ξ ToMessage( _::OptionValues* p )${ SET(set_allocated_options); }
	Ξ ToMessage( _::EResults t, int v )${ auto p = new _::MessageValue(); p->set_type( t ); p->set_int_value( v ); SET(set_allocated_message); }
	Ξ ToMessage( _::EResults t, string v )${ auto p = new _::MessageValue(); p->set_type( t ); p->set_string_value( move(v) ); SET(set_allocated_message); }
	Ξ ToMessage( _::EResults t, uint32 client, ContractPK c, double v )${ auto p = new _::ContractValue(); p->set_type( t ); p->set_request_id( client ); p->set_value( v ); p->set_contract_id( c ); SET(set_allocated_contract_value); }
	Ξ ToMessage( uint32 c, Edgar::Proto::Investors* p )${ p->set_request_id( c ); SET(set_allocated_investors); }
	Ξ ToMessage( int id, _::Tweets* p )${ p->set_request_id( id ); SET(set_allocated_tweets); }
	Ξ ToMessage( int id, _::TweetAuthors* p )${ p->set_request_id( id ); SET( set_allocated_tweet_authors ); }
	Ξ ToMessage( int id, _::OptionExchanges* p )${ p->set_request_id( id ); SET(set_allocated_option_exchanges); }
	Ξ ToMessage( int id, _::OpenOrder* p )${ p->set_request_id( id ); SET(set_allocated_open_order); }
	Ξ ToMessage( _::Orders* p )${ SET( set_allocated_orders ); }
	Ξ ToMessage( ETickType t, ContractPK c, uint v )${ auto p = new TickSize(); p->set_request_id( c ); p->set_tick_type( t ); p->set_size( (double)v ); SET( set_allocated_tick_size ); }
	Ξ ToRatioMessage( const std::map<string,double>& x, uint32 c )${ auto p = new _::Fundamentals(); p->set_request_id( c ); for( let& [n,v] : x ) (*p->mutable_values())[n] = v; SET(set_allocated_fundamentals); }
	Ξ ToMessage( ContractStats* p, uint32 c )${ p->set_request_id(c); SET( set_allocated_contract_stats ); }
}
#undef $
#undef SET
#undef _
#undef let