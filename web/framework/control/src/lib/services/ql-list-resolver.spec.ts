import { describe, it, expect } from 'vitest';
import { Operator, View } from '../model/ql/view';
import { TableSchema } from '../model/ql/schema/table-schema';
import { QLListResolver, TableSettings } from './ql-list-resolver';

const schema = new TableSchema( {
	name: "Thing",
	fields: [
		{ name: "id", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "ID" } } },
		{ name: "name", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "target", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
		{ name: "kind", type: { kind: "SCALAR", name: "String" } },
		{ name: "deleted", type: { kind: "SCALAR", name: "DateTime" } },
		{ name: "description", type: { kind: "SCALAR", name: "String" } }
	]
} );
const displayed = ( v:View )=>v.fields.filter( f=>f.displayed ).map( f=>f.name );

//A route declares its list page's system views in TableSettings:  the default one from columns/sort under `viewName`, and
//`views` beside it - each a named, optionally filtered variant that inherits whatever it leaves unset.
describe( 'QLListResolver.systemViews', ()=>{
	it( 'is just the unnamed default view when the route declares none', ()=>{
		const views = QLListResolver.systemViews( schema, {columns: ["name", "description"]} );
		expect( views.map(v=>v.name) ).toEqual( [undefined] );
		expect( views[0].isSystem ).toBe( true );
	} );

	it( 'names the default view and appends the declared ones, all as system views', ()=>{
		const settings:TableSettings = { viewName: "all", columns: ["name", "kind", "description"], views: [
			{ name: "Kinds", columns: ["name", "kind"], filters: [{name: "kind", value: ["a", "b"]}] },
			{ name: "Newest", sort: "target", filters: [{name: "kind", operator: Operator.NotIn, value: ["<null>"]}] }
		] };
		const views = QLListResolver.systemViews( schema, settings );
		expect( views.map(v=>v.name) ).toEqual( ["all", "Kinds", "Newest"] );
		expect( views.every(v=>v.isSystem) ).toBe( true );
	} );

	it( 'gives a declared view the default columns and sort unless it sets its own', ()=>{
		const [all, kinds, newest] = QLListResolver.systemViews( schema, { columns: ["name", "kind", "description"], views: [
			{ name: "Kinds", columns: ["name", "kind"] },
			{ name: "Newest", sort: "target" }
		] } );
		expect( displayed(kinds) ).toEqual( ["name", "kind"] );
		expect( kinds.sort ).toEqual( all.sort );
		expect( kinds.sort ).not.toBe( all.sort );//its own array - a header sort on one view must not reach the other
		expect( displayed(newest) ).toEqual( displayed(all) );
		expect( newest.sort ).toEqual( [{active: "target", direction: "asc"}] );
	} );

	it( 'turns a declared filter into the query the settings panel would have built', ()=>{
		const [, kinds] = QLListResolver.systemViews( schema, { columns: ["name"], views: [
			{ name: "Kinds", filters: [{name: "kind", value: ["a", "<null>"]}] }
		] } );
		expect( kinds.fieldFilters ).toHaveLength( 1 );
		expect( kinds.fieldFilters[0].field.name ).toBe( "kind" );
		expect( kinds.fieldFilters[0].filter.operator ).toBe( Operator.In );
		const q = kinds.query( false, 0 );
		expect( q.text ).toContain( "kind:$kind" );
		expect( q.vars["kind"] ).toEqual( ["a", null] );//"<null>" leaves as a JSON null, as the panel's does
	} );

	it( 'refuses a filter on a column the schema does not have', ()=>{
		expect( ()=>QLListResolver.systemViews(schema, { columns: ["name"], views: [{name: "Bad", filters: [{name: "nope", value: [1]}]}] }) ).toThrow( /nope/ );
	} );
} );
