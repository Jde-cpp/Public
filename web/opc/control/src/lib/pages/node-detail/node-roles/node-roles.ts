import { Component, computed, model, Inject, OnDestroy, OnInit, signal, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MatTable, MatTableModule } from "@angular/material/table";
import { MatSortModule, Sort } from "@angular/material/sort";
import { AccessService } from "jde-access";
import { IErrorService } from "jde-framework";
import { Server } from '../../../model/Server';
import { UaNode } from '../../../model/Node';
import { Gateway } from '../../../services/gateway.service';
import { EAccess, EWriteAccess } from '../../../model/types';

type NodeRole = { id: number; name: string; target:string, accessRights: EAccess|string[]; writeRights: EWriteAccess|string[]; deleted: Date; };
@Component( {
	selector: 'node-roles',
	templateUrl: './node-roles.html',
	styleUrls: ['./node-roles.scss'],
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [CommonModule, MatSortModule, MatTableModule]
})
export class NodeRoles implements OnInit {
	constructor( @Inject('IErrorService') private cnsle: IErrorService )
	{}

	async ngOnInit(): Promise<void> {
		const vars = { opc: this.opcTarget(), id: this.node().id };
		this.roles = await this.gatewayService().query(`node( opc: $opc, id: $id ){ roles{ id target name accessRights writeRights deleted } }`, vars, (m)=>console.log(m) );
	}

	sortData($event){
		this.roles = this.roles.sort((a,b)=>{
			const colName = $event.active;
			let y:number;
			//if( ["target", "name", "deleted"].includes(colName) ){
			y = a[colName].localeCompare( b[colName] );
			// }else{
			// 	if( colName=="accessRights" ){
			// 		a
			// 	let right = <number><any>EAccess[colName];
			// 	let value = (x:Permission)=>{ return this.isAllowed(x, right) ? 1 : this.isDenied(x, right) ? -1 : 0; };
			// 	y = value(b) - value(a);
			// }
			return $event.direction=="asc" ? y : -y;
		});
		if( this.table )
			this.table.renderRows();
	}

	gatewayService = model.required<Gateway>();
	node = model.required<UaNode>();
	opcTarget = model.required<string>();
	roles: NodeRole[] = [];
	get sort():Sort{ return {active: "name", direction: "asc"}; }
	@ViewChild('mainTable',{static: false}) table!:MatTable<NodeRole>;

}