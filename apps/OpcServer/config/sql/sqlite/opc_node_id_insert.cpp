#include <sqlite3.h>
#include <boost/crc.hpp>
#include "opcProcs.h"
#include <jde/db/DBException.h>

#define let const auto

//Twin of ../mysql/opc_node_id_insert.sql.
//	params: [0]=_ns, [1]=_number, [2]=_string, [3]=_guid, [4]=_bytes, [5]=_namespace_uri, [6]=_server_index,
//	[7]=_is_global; out _node_id returned as the result row.
namespace Jde::DB::Sqlite::OpcProcs{
	//mysql CRC32 = CRC-32/ISO-HDLC, byte-wise over the argument - boost::crc_32_type is the same algorithm.
	Ω crc32( const void* p, uint size )ι->uint{
		boost::crc_32_type crc;
		crc.process_bytes( p, size );
		return crc.checksum();
	}
	//mysql BIN_TO_UUID: lowercase hex of the 16 bytes in order, dashed 8-4-4-4-12.
	Ω toUuidString( const vector<uint8_t>& bytes, SL sl )ε->string{
		THROW_IFSL( bytes.size()!=16, "Guid blob is {} bytes, expected 16.", bytes.size() );
		string y; y.reserve( 36 );
		for( uint i=0; i<16; ++i ){
			if( i==4 || i==6 || i==8 || i==10 )
				y += '-';
			y += Ƒ( "{:02x}", bytes[i] );
		}
		return y;
	}

	α NodeIdInsert( sqlite3& db, const Value& ns, const Value& number, const Value& string_, const Value& guid, const Value& bytes, const Value& namespaceUri, const Value& serverIndex, const Value& isGlobal, SL sl )ε->uint{
		uint nodeId = ns.get_number<uint>( sl ) << 32;
		if( !number.is_null() )
			nodeId |= number.get_number<uint>( sl );
		else if( !string_.is_null() )
			nodeId |= crc32( string_.get_string().data(), string_.get_string().size() );
		else if( !guid.is_null() ){
			let uuid = toUuidString( guid.get_bytes(), sl );
			nodeId |= crc32( uuid.data(), uuid.size() );
		}
		else if( !bytes.is_null() )
			nodeId |= crc32( bytes.get_bytes().data(), bytes.get_bytes().size() ); //mysql casts to char - same bytes.
		ExecuteStatement( db, "insert into opc_node_ids( node_id, ns, number, string, guid, bytes, namespace_uri, server_index, is_global ) values( ?, ?, ?, ?, ?, ?, ?, ?, ? )",
			{Value{nodeId}, ns, number, string_, guid, bytes, namespaceUri, serverIndex, isGlobal}, nullptr, sl );
		return nodeId;
	}

	α EnsureDataTypeNodeId( sqlite3& db, const Value& dataTypeId, SL sl )ε->void{
		if( dataTypeId.is_null() || dataTypeId.get_number<uint>(sl)>32750 )
			return;
		if( let count = ScalarUInt(db, "select count(*) from opc_node_ids where node_id=?", {dataTypeId}, sl); count && *count==0 )
			NodeIdInsert( db, Value{(uint)0}, dataTypeId, Value{}, Value{}, Value{}, Value{}, Value{}, Value{}, sl );
	}

	α RegisterOpcNodeIdInsert()ι->void{
		RegisterProc( "opc_node_id_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let nodeId = NodeIdInsert( db, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], sl );
			if( onRow )
				(*onRow)( Row{ {Value{nodeId}} } ); //out _node_id
			return 1;
		});
	}
}
