import { describe, it, expect } from 'vitest';
import { View } from './View';
import { TableSchema } from './schema/TableSchema';

//Mirrors the gateway's ServerConnection: introspected DB columns plus the grafted opcSessions OBJECT (config/introspection/serverConnection.jsonnet).
const schema = new TableSchema( {
	name: "ServerConnection",
	fields: [
		{ name: "id", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "ID" } } },
		{ name: "name", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "target", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "url", type: { kind: "SCALAR", name: "String" } },
		{ name: "deleted", type: { kind: "SCALAR", name: "DateTime" } },
		{ name: "description", type: { kind: "SCALAR", name: "String" } },
		{ name: "provider", type: { kind: "OBJECT", name: "Provider" } },
		{ name: "opcSessions", type: { kind: "OBJECT", name: "OpcSessions" } }
	]
} );

describe( "View.query", ()=>{
	it( "emits an explicit sub-selection for an OBJECT column", ()=>{
		const view = new View( {columns: ["name", "url", {name:"opcSessions", displayName:"Sessions", selection:"count"}], sort: "name"}, schema );
		const query = view.query( false, 0 );
		expect( query.text ).toContain( "opcSessions{count}" );
		expect( query.text ).toContain( "serverConnections(" );
		expect( query.text ).not.toContain( "opcSessions " );//never the bare name - the server rejects a selection-less object field.
	} );
	it( "defaults an OBJECT column to the {id name} convention", ()=>{
		const view = new View( {columns: ["name", "provider"], sort: "name"}, schema );
		expect( view.query(false, 0).text ).toContain( "provider{id name}" );
	} );
	it( "round-trips selection through toJson", ()=>{
		const view = new View( {columns: [{name:"opcSessions", selection:"count"}], sort: "name"}, schema );
		const field = view.fields.find( f=>f.name=="opcSessions" )!;
		expect( field.toJson().selection ).toBe( "count" );
	} );
} );
