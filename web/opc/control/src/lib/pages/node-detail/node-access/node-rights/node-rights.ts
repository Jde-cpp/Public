import { Component, computed, ViewChild, input, output, effect } from '@angular/core';
import { CommonModule, KeyValue } from '@angular/common';
import { MatCheckboxModule } from "@angular/material/checkbox";
import { MatTable, MatTableModule } from "@angular/material/table";

import { MatSortModule, Sort } from "@angular/material/sort";
import { EAccess, EWriteAccess } from '../../../../model/types';
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

	static toAccess( rights:number ):EAccess{
		return rights & EAccess.All;
	}
	static toWrite( rights:number ):EWriteAccess{
		return rights >> 32;
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
		if( this.isWriteRights() )
			rights = rights << 32;
		this.toggleEmitter.emit( { role: role, rights: rights } );
	}
	sortData($event){
		this.data = this.data.slice().sort((a,b)=>{
			const colName = $event.active;
			let y:number;
			y = a[colName].localeCompare( b[colName] );
			return $event.direction=="asc" ? y : -y;
		});
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
	isWriteRights = computed<boolean>( ()=>this.rights()==EWriteAccess );
	roles = input.required<RolePermission[]>();
	data:RolePermission[] = [];
	get sort():Sort{ return {active: "roleName", direction: "asc"}; }
	@ViewChild('mainTable',{static: false}) table!:MatTable<RolePermission>;
}