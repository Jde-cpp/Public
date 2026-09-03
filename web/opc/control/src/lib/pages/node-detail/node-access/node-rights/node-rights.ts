import { Component, ViewChild, input, output, effect } from '@angular/core';
import { CommonModule, KeyValue } from '@angular/common';
import { MatCheckboxModule } from "@angular/material/checkbox";
import { MatTable, MatTableModule } from "@angular/material/table";

import { MatSortModule, Sort } from "@angular/material/sort";
import { RolePermission } from '../node-access';

@Component( {
	selector: 'node-rights',
	templateUrl: './node-rights.html',
	styleUrls: ['./node-rights.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, MatSortModule, MatTableModule, MatCheckboxModule]
})
export class NodeRights {
	constructor(){
		effect( ()=>{
			this.data = this.roles().slice();
			this.sortData( this.sort );
		});
	}

	static isDenied( role: RolePermission, rights:number ):boolean{
		let denied = role.denied ?? 0;
		return rights!=0 && (denied & rights)==rights;
	}
	static isAllowed( role: RolePermission, rights:number ):boolean{
		let allowed = role.allowed ?? 0;
		let y = ( rights==0 && !allowed && !role.denied )
			|| (rights!=0 && (allowed & rights)==rights);
		return y;
	}
	static isAvailable( role: RolePermission, rights:number ):boolean{
		return true;
	}
	toggle( role: RolePermission, rights:number ):void{
		this.toggleEmitter.emit( { role: role, rights: rights } );
	}
	//Every header carries mat-sort-header, but only 'roleName' is a field on the row:  the rights columns are named after the
	//RIGHT (available.value) and 'inherited' is not on RolePermission at all, so `a[colName]` was undefined and localeCompare
	//threw on every header but Role.  Sort a rights column by what the row shows in it, and keep roleName as the tiebreak so
	//the order inside a group stays the alphabetical one.
	sortData( $event:Sort ){
		const colName = $event.active;
		const right = this.available.find( kv=>kv.value==colName );
		const rank = ( role:RolePermission ):number=>
			right ? (NodeRights.isAllowed(role, right.key) ? 0 : NodeRights.isDenied(role, right.key) ? 1 : 2)
			: (<any>role).inherited ? 0 : 1;//'inherited' is a stub column - undefined must sort, not throw
		const compare = colName=="roleName"
			? ( a:RolePermission, b:RolePermission )=>a.roleName.localeCompare( b.roleName )
			: ( a:RolePermission, b:RolePermission )=>rank(a)-rank(b) || a.roleName.localeCompare( b.roleName );
		this.data = this.data.slice().sort( (a,b)=>$event.direction=="asc" ? compare(a,b) : -compare(a,b) );
		if( this.table )
			this.table.renderRows();
	}
	get available(): KeyValue<number,string>[] {
		let y:KeyValue<number,string>[] = [];
		for( let right in this.rights() ){
			let key = Number(right);
			if( !isNaN(key) )
				y.push( { key: key, value: this.rights()[right] } );
		}
		return y;
	}
	get displayedColumnNames(): string[] {
		let y = ['roleName', 'inherited'];
		for( let kv of this.available )
			y.push( kv.value );
		return y;
	}

	toggleEmitter = output<{role: RolePermission, rights:number}>(); //
	rights = input<any>();
	self = NodeRights;
	get allRights(): number{ return this.available[this.available.length-1].key; }
	roles = input.required<RolePermission[]>();
	data:RolePermission[] = [];
	get sort():Sort{ return {active: "roleName", direction: "asc"}; }
	@ViewChild('mainTable',{static: false}) table!:MatTable<RolePermission>;
}