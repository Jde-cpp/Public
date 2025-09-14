import { SelectionModel, SelectionChange } from '@angular/cdk/collections';
import {Component, computed, inject, Inject, model, OnDestroy, OnInit, signal} from '@angular/core';
import {MatButtonModule} from '@angular/material/button';
import {MatCheckboxChange, MatCheckboxModule} from '@angular/material/checkbox';
import {RouterModule, ActivatedRoute, Router} from '@angular/router';
import { Gateway, GatewayService, SubscriptionResult } from '../../services/gateway.service';
import { IErrorService, IProfile, subscribe} from 'jde-framework'
import { ETypes } from '../../model/types';
import {  MatTableModule } from '@angular/material/table';
import { Subscription } from 'rxjs';
import { ComponentPageTitle } from 'jde-spa';
import { MatToolbarModule } from '@angular/material/toolbar';
import { NodePageData } from '../../services/resolvers/node.resolver';
import { NodeRoute } from '../../model/NodeRoute';
import { OpcNodeRouteService } from '../../services/routes/opc-node-route.service';
import { Value, toString } from '../../model/Value';
import { ENodeClass, Variable, UaNode }  from '../../model/Node';
import { ServerCnnctn } from '../../model/ServerCnnctn';

@Component({
  selector: 'node-detail',
  templateUrl: './node-detail.html',
  styleUrls: ['./node-detail.scss'],
  standalone: true,
		imports: [RouterModule,MatButtonModule,MatCheckboxModule,MatTableModule,MatToolbarModule]
})
export class NodeDetail implements OnInit, OnDestroy {
	constructor(
		@Inject('GatewayService') private gatewayService:GatewayService,
		private route: ActivatedRoute,
		@Inject('IErrorService') private snackbar: IErrorService,
		private componentPageTitle:ComponentPageTitle )
	{}

	async ngOnInit() {
		this.route.data.subscribe( (data)=>{
			this.pageData = data["pageData"];
			this.sideNav.set( this.pageData.route );
			this.nodes?.filter( (n:UaNode)=>this.profile.subscriptions.find((s)=>s.equals(n)) ).forEach( (n)=>this.selections.select(n) );
			this.isLoading.set( false );
			this.componentPageTitle.title = this.connection.name + (this.node.id==85 ? '' : `/${this.node.name}`);
		});
		this.selections.changed.subscribe( this.onSubscriptionChange.bind(this) );
	}

  ngOnDestroy() {
		this.selections.clear();
		this.selections.changed.unsubscribe();
		this.pageData.route.settings.save();
		this.subscription = null;
  }

	async retrieveSnapshot(){
		this.retrievingSnapshot.set( true );
		this.variables.forEach( r=>r.value=null );
		var snapshots = await this._iot.snapshot( this.cnnctnTarget, this.variables );
		for( let [node,value] of snapshots ){
			let variable = this.variables.find( (n)=>n.equals(node) );
			if( variable )
				variable.value = value;
		}
		this.retrievingSnapshot.set( false );
	}

	toObject( x:ENodeClass ):string{ return ENodeClass[x]; }
	toString( value:Value ){ return toString(value); }
  checkboxLabel(row?: UaNode): string {
		return row
			? `${this.selections.isSelected(row) ? 'deselect' : 'select'} ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
  }
	async onSubscriptionChange( r:SelectionChange<UaNode> ){
		if( r.added.length>0 ){
			try {
				let nodes = r.added.map( r=>r.nodeId );
				this.profile.subscriptions.push( ...nodes );
				if( !this.subscription){
					this.subscription = this._iot.subscribe( this.cnnctnTarget, nodes, this.Key ).subscribe({
						next:(value: SubscriptionResult) =>{
							this.variables.find( (r)=>r.nodeId.equals(value.node) ).value = value.value;
						},
						error:(e: Error) =>{
							this.snackbar.exception( e, (m)=>console.log(m) );
						},
						complete:()=>{ console.debug( "complete" );}
					});
				}
				else
					this._iot.addToSubscription( this.cnnctnTarget, nodes, this.Key );
			} catch (e) {
				this.snackbar.error( e["error"], (m)=>console.log(m) );
			}
		}
		if( r.removed.length>0 ){
			let nodes = r.removed.map( r=>r.nodeId );
			this.profile.subscriptions = this.profile.subscriptions.filter( s=>!nodes.includes(s) );
			if( !this.selections.selected.length )
				this.subscription = null;
			else{
				try{
					this._iot.unsubscribe( this.cnnctnTarget, nodes, this.Key );
				}
				catch (e) {
					this.snackbar.error( e["error"], (m)=>console.log(m) );
				}
			}
		}
	}

  toggleAllRows() {
		if( this.isAllSelected() )
			this.selections.clear();
		else
			this.selections.select(...this.nodes);
  }

	async toggleValue( e:MatCheckboxChange, x:Variable ){
		e.source.checked = !e.source.checked;
		try {
			x.value = await this._iot.write( this.cnnctnTarget, x.nodeId, !x.value );
		}
		catch (e) {
			this.snackbar.exception( e, (m)=>console.log(m) );
		}
	}
	async changeDouble( e:Event, x:Variable ){
		try {
			x.value = await this._iot.write( this.cnnctnTarget, x.nodeId, +e.target["value"] );
		}
		catch (e) {
			this.snackbar.exception( e, (m)=>console.log(m) );
		}
	}
	async changeString( e:Event, n:Variable ){
		try {
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, e.target["value"] );
		}
		catch (err) {
			e.target["value"] = n.value;
			this.snackbar.exception( err, (m)=>console.error(m) );
		}
	}
	routerLink(n:UaNode):string[]{
		return [`./${n.browse}`];
	}
	test(r:UaNode){ debugger;}
	get columns():string[]{ return this.profile.columns; }
	ETypes = ETypes;
	get _iot():Gateway{ return this.pageData.gateway; }
	isAllSelected = computed<boolean>( ()=>{ return this.selections.selected.length==this.nodes.length; } );
	isLoading = signal<boolean>( true );
	get Key():string{ return this.pageData.route.profileKey; }
	get node(){ return this.sideNav().node; }
	get nodeId(){ return this.node.nodeId; }
	get connection():ServerCnnctn{ return this.pageData.connection; }
	get cnnctnTarget():string{ return this.connection.target; }
	pageData:NodePageData;
	//get parent():types.ExpandedNode{ return this.pageData.parent; }
	get profile(){ return this.pageData.route.profile; }
	get nodes(){ if(!this.pageData) debugger; return this.pageData?.nodes; }
	get variables():Variable[]{ return <Variable[]>this.nodes.filter((x)=>x.nodeClass==ENodeClass.Variable); }
	retrievingSnapshot = signal<boolean>( false );
	routerSubscription:Subscription;
	selections = new SelectionModel<UaNode>(true, []);
	get showSnapshot():boolean{ return this.visibleColumns.includes("snapshot");}
	//#sideNav = signal<NodeRoute>( null );
	sideNav = model.required<NodeRoute>();
	get sort(){ return this.profile.sort; };
	get subscription(){return this.#subscription;} #subscription:Subscription;
	set subscription(x){ if(!x && this.subscription) this.subscription.unsubscribe(); this.#subscription=x; }
	viewPromise:Promise<boolean>;
	get visibleColumns(){ return this.profile.visibleColumns; }

	#routeService = inject( OpcNodeRouteService );
}