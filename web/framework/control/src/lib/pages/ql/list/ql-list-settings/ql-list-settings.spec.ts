if( typeof globalThis.localStorage=="undefined" ){
	const backing = new Map<string,string>();
	(globalThis as any).localStorage = {
		getItem: ( k:string )=>backing.has(k) ? backing.get(k)! : null,
		setItem: ( k:string, v:string )=>{ backing.set(k, String(v)); },
		removeItem: ( k:string )=>{ backing.delete(k); },
		clear: ()=>backing.clear()
	};
}
import { TestBed } from '@angular/core/testing';
import { TableSchema } from '../../../../model/ql/schema/table-schema';
import { View } from '../../../../model/ql/view';
import { QLListSettings } from './ql-list-settings';

const schema = new TableSchema( { name: "User", fields: [
	{ name: "id", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "ID" } } },
	{ name: "name", type: { kind: "NON_NULL", name: null, ofType: { kind: "SCALAR", name: "String" } } },
	{ name: "target", type: { kind: "SCALAR", name: "String" } },
	{ name: "deleted", type: { kind: "SCALAR", name: "DateTime" } }
] } );

//review3 C10: the View Name field was the workspace's last [(ngModel)].  It binds [value]/(input) against the `name`
//signal now - the same signal disableSave() and getView() already read - so this pins that the round trip still works.
describe( 'QLListSettings view name', ()=>{
	const create = ()=>{
		TestBed.resetTestingModule();
		TestBed.configureTestingModule({});
		const fixture = TestBed.createComponent( QLListSettings );
		const view = new View( {columns: ["name"], sort: "name"}, schema );
		view.name = "Mine";//the settings-shaped ctor arg carries no name; onViewSave assigns it the same way
		fixture.componentRef.setInput( 'view', view );
		fixture.componentRef.setInput( 'schema', schema );
		fixture.componentRef.setInput( 'columns', {name: "Name"} );
		fixture.componentRef.setInput( 'suggestions', {} );
		fixture.componentInstance.ngOnInit();
		fixture.detectChanges();
		return fixture;
	};

	const input = ( fixture:any ):HTMLInputElement=>fixture.nativeElement.querySelector( 'input[matInput]' );

	it( 'renders the current name', ()=>{
		expect( input(create()).value ).toBe( "Mine" );
	} );

	it( 'writes typing back into the signal', ()=>{
		const fixture = create();
		const el = input( fixture );
		el.value = "Renamed";
		el.dispatchEvent( new Event('input') );
		expect( fixture.componentInstance.name() ).toBe( "Renamed" );//what [(ngModel)] used to do
	} );

	it( 'selects the text on focus', ()=>{
		const fixture = create();
		const el = input( fixture );
		el.dispatchEvent( new FocusEvent('focus') );
		expect( el.selectionStart ).toBe( 0 );
		expect( el.selectionEnd ).toBe( "Mine".length );
	} );
} );
