import { TestBed } from '@angular/core/testing';
import { Rights } from 'jde-access';
import { RolePermission } from '../node-access';
import { NodeRights } from './node-rights';

const role = ( name:string, allowed:number, denied:number ):RolePermission =>
	({roleId: 1, roleName: name, permissionId: 0, allowed, denied, resourceId: 0, criteria: ""});

//The rows the page shows: 'ops' is allowed Read, 'dev' is denied it, 'qa' has neither.
const rows = [ role("qa", Rights.None, Rights.None), role("ops", Rights.Read, Rights.None), role("dev", Rights.None, Rights.Read) ];

describe( 'NodeRights.sortData', ()=>{
	let table:NodeRights;
	beforeEach( ()=>{
		TestBed.configureTestingModule({});
		const fixture = TestBed.createComponent( NodeRights );
		fixture.componentRef.setInput( 'roles', rows );
		fixture.componentRef.setInput( 'rights', Rights );
		fixture.detectChanges();//the constructor effect copies roles() into data and sorts by roleName
		table = fixture.componentInstance;
	} );
	const names = ()=>table.data.map( r=>r.roleName );

	it( 'sorts by role name, both directions', ()=>{
		table.sortData( {active: "roleName", direction: "asc"} );
		expect( names() ).toEqual( ["dev", "ops", "qa"] );
		table.sortData( {active: "roleName", direction: "desc"} );
		expect( names() ).toEqual( ["qa", "ops", "dev"] );
	} );

	//angular-review3 L1: the column names are the RIGHT names, not fields on the row, so `a[colName]` was undefined and
	//localeCompare threw on every header but Role - 'inherited' included, which is not on RolePermission at all.
	it( 'sorts a rights column by what the row shows: allowed, then denied, then neither', ()=>{
		table.sortData( {active: "Read", direction: "asc"} );
		expect( names() ).toEqual( ["ops", "dev", "qa"] );
		table.sortData( {active: "Read", direction: "desc"} );
		expect( names() ).toEqual( ["qa", "dev", "ops"] );
	} );

	it( 'breaks ties on a rights column by role name', ()=>{
		table.sortData( {active: "Update", direction: "asc"} );//nobody has Update - every row ranks the same
		expect( names() ).toEqual( ["dev", "ops", "qa"] );
	} );

	it( 'does not throw on the inherited stub column', ()=>{
		expect( ()=>table.sortData({active: "inherited", direction: "asc"}) ).not.toThrow();
		expect( names() ).toEqual( ["dev", "ops", "qa"] );
	} );
} );
