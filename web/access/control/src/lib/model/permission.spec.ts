import { MutationType } from 'jde-framework';
import { Permission, Rights } from './permission';

const permission = ( id:number, allowed:Rights, denied:Rights )=>new Permission( {id, allowed, denied, resource:{id:9, target:"nodeIds"}} );

//The rule node-access's onToggle was aligned to in angular-review3 #2: Remove-vs-change is decided from the RESULTING
//row, so clearing ONE side of a two-sided permission is an update, not a delete of the whole permissionRight.
describe( 'Permission.roleMutations', ()=>{
	it( 'removes the permissionRight only when the row ends with no rights at all', ()=>{
		const mutations = Permission.roleMutations( 5, [permission(3, Rights.None, Rights.None)], [permission(3, Rights.Read, Rights.None)] );
		expect( mutations ).toHaveLength( 1 );
		expect( mutations[0].type ).toBe( MutationType.Remove );
		expect( mutations[0].toString() ).toContain( "removeRole" );
	} );

	it( 'clearing the denied side keeps the row and updates it', ()=>{
		const mutations = Permission.roleMutations( 5, [permission(3, Rights.Read, Rights.None)], [permission(3, Rights.Read, Rights.Update)] );
		expect( mutations ).toHaveLength( 1 );
		expect( mutations[0].type ).toBe( MutationType.Update );
		expect( mutations[0].args ).toEqual( {denied: Rights.None} );//only the side that changed travels
	} );

	it( 'clearing the allowed side keeps the row too', ()=>{
		const mutations = Permission.roleMutations( 5, [permission(3, Rights.None, Rights.Update)], [permission(3, Rights.Read, Rights.Update)] );
		expect( mutations[0].type ).toBe( MutationType.Update );
		expect( mutations[0].args ).toEqual( {allowed: Rights.None} );
	} );

	it( 'emits nothing when neither side moved', ()=>{
		expect( Permission.roleMutations( 5, [permission(3, Rights.Read, Rights.Update)], [permission(3, Rights.Read, Rights.Update)] ) ).toEqual( [] );
	} );

	it( 'adds a row the role did not have', ()=>{
		const mutations = Permission.roleMutations( 5, [new Permission({allowed:Rights.Read, denied:Rights.None, resource:{id:9, target:"nodeIds"}})], [] );
		expect( mutations[0].type ).toBe( MutationType.Add );
		expect( mutations[0].args.permissionRight ).toMatchObject( {allowed: Rights.Read, denied: Rights.None} );
	} );
} );
