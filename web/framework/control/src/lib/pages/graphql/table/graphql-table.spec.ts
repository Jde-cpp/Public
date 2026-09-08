import { describe, it, expect } from 'vitest';
import { signal } from '@angular/core';
import { TestBed } from '@angular/core/testing';
import { SelectionModel } from '@angular/cdk/collections';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { TableSchema } from '../../../model/ql/schema/table-schema';
import { Flex, Style, View } from '../../../model/ql/view';
import { GraphQLTable } from './graphql-table';

//The gateway's Connections list: a text column, and the grafted count that reads as a number.
const schema = new TableSchema( { name: "ServerConnection", enums: new Map(), fields: [
	{ name: "name", type: {kind: "NON_NULL", ofType: {kind: "SCALAR", name: "String"}} },
	{ name: "opcSessions", type: {kind: "OBJECT", name: "OpcSessions"} }
] } );

const create = ( columns:any[] )=>{
	TestBed.resetTestingModule();
	TestBed.configureTestingModule({ providers: [{ provide: SnackbarService, useValue: {} }] });
	const fixture = TestBed.createComponent( GraphQLTable );
	fixture.componentRef.setInput( 'dataSource', signal([]) );
	fixture.componentRef.setInput( 'displayedFields', new View({columns, sort: "name"}, schema).fields );
	fixture.componentRef.setInput( 'selections', new SelectionModel<any>(false, []) );
	return fixture.componentInstance;
};

//A heading has to sit over its values.  Both cells are flex containers and the same columnStyle() output goes on each,
//so an alignment that only set text-align would leave the content wherever justify-content had already put it.
describe( 'GraphQLTable.columnStyle', ()=>{
	it( 'expands a right alignment to both flex and text', ()=>{
		const style = create( ["name", {name:"opcSessions", selection:"count", style: new Style({align:"right"})}] ).columnStyle( "opcSessions" );
		expect( style ).toEqual( {justifyContent: "flex-end", textAlign: "right"} );
	} );

	it( 'centres on center and falls back to the start for left', ()=>{
		const centred = create( [{name:"name", style: new Style({align:"center"})}] ).columnStyle( "name" );
		expect( centred ).toEqual( {justifyContent: "center", textAlign: "center"} );
		const left = create( [{name:"name", style: new Style({align:"left"})}] ).columnStyle( "name" );
		expect( left ).toEqual( {justifyContent: "flex-start", textAlign: "left"} );
	} );

	it( 'keeps the width alongside the alignment', ()=>{
		const style = create( [{name:"name", style: new Style({flex: new Flex(130), align:"right"})}] ).columnStyle( "name" );
		expect( style ).toEqual( {flex: "0 0 130px", justifyContent: "flex-end", textAlign: "right"} );
	} );

	it( 'is empty for a column with no style, so the stylesheet decides', ()=>{
		expect( create(["name"]).columnStyle("name") ).toEqual( {} );
	} );
} );

//The column names the tones and the stylesheet owns the colours, so a value the column did not list has to fall back to the
//theme's default chip rather than pick up someone else's colour.
describe( 'GraphQLTable chip columns', ()=>{
	const status = {name:"opcSessions", selection:"count", chip:{Connected:"ok", Idle:"neutral", Error:"error"} as const};
	it( 'maps each listed value to its tone', ()=>{
		const table = create( ["name", status] );
		expect( table.chipClass("opcSessions", {opcSessions:{name:"Connected"}} as any) ).toBe( "chip-ok" );
		expect( table.chipClass("opcSessions", {opcSessions:{name:"Idle"}} as any) ).toBe( "chip-neutral" );
		expect( table.chipClass("opcSessions", {opcSessions:{name:"Error"}} as any) ).toBe( "chip-error" );
	} );

	it( 'gives an unlisted or missing value no tone', ()=>{
		const table = create( ["name", status] );
		expect( table.chipClass("opcSessions", {opcSessions:{name:"Draining"}} as any) ).toBe( "" );
		expect( table.chipClass("opcSessions", {} as any) ).toBe( "" );
	} );

	it( 'gives a column with no chip setting no tone', ()=>{
		expect( create(["name"]).chipClass("name", {name:"Connected"} as any) ).toBe( "" );
	} );

	//Both buckets feed matColumnDef, so a chip column left in its original one is declared twice and the table throws.
	it( 'takes the column out of the bucket it would otherwise render in', ()=>{
		const table = create( ["name", status] );
		expect( table.chipColumnNames ).toEqual( ["opcSessions"] );
		expect( table.objectColumnNames ).toEqual( [] );
		expect( table.stringColumnNames ).toContain( "name" );
	} );

	it( 'leaves the buckets alone when nothing asks for a chip', ()=>{
		const table = create( ["name", {name:"opcSessions", selection:"count"}] );
		expect( table.chipColumnNames ).toEqual( [] );
		expect( table.objectColumnNames ).toEqual( ["opcSessions"] );
	} );
} );
