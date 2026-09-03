import { describe, it, expect } from 'vitest';
import { Operator, View } from './view';
import { TableSchema } from './schema/table-schema';

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

//A persisted view outlives the schema (groupings->groups already happened once):  the revived view has to prune, not throw -
//the throw landed inside loadViews and rejected QLListResolver, killing the page even on the default view.
describe( "View revival against a changed schema", ()=>{
	const serialized = ()=>(<any>{
		name: "Mine",
		collectionName: "serverConnections",
		fields: [ {name: "name"}, {name: "groupings"} ],
		filters: [ {name: "groupings", filter: {operator: Operator.In, value: ["a"]}} ],
		showSelector: false,
		sort: [ {active: "name", direction: "asc"} ]
	});
	it( "drops a displayed column the schema no longer has", ()=>{
		const view = new View( serialized(), schema, [] );
		const names = view.fields.map( f=>f.name );
		expect( names ).toContain( "name" );
		expect( names ).not.toContain( "groupings" );
	} );
	it( "drops a filter on a column the schema no longer has", ()=>{
		const view = new View( serialized(), schema, [] );
		expect( view.fieldFilters ).toHaveLength( 0 );
	} );
	it( "still builds a query afterwards", ()=>{
		const view = new View( serialized(), schema, [] );
		const query = view.query( false, 0 );
		expect( query.text ).toContain( "serverConnections(" );
		expect( query.text ).not.toContain( "groupings" );
	} );
	it( "keeps a filter whose column survives", ()=>{
		const args = serialized();
		args.filters = [ {name: "url", filter: {operator: Operator.In, value: ["a"]}} ];
		const view = new View( args, schema, [] );
		expect( view.fieldFilters.map(ff=>ff.field.name) ).toEqual( ["url"] );
	} );
} );
