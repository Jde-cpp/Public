import { SelectionModel, SelectionChange } from '@angular/cdk/collections';
import {ChangeDetectorRef, Component, inject, Inject, model, OnDestroy, OnInit, signal} from '@angular/core';
import {MatButtonModule} from '@angular/material/button';
import {MatCheckboxChange, MatCheckboxModule} from '@angular/material/checkbox';
import {MatSelectChange, MatSelectModule} from '@angular/material/select';
import { Sort } from "@angular/material/sort";
import {RouterModule, ActivatedRoute, Router} from '@angular/router';
import { Gateway, GatewayService, SubscriptionResult } from '../../../services/gateway.service';
import { ProfileStore } from 'jde-spa';
import { DateUtils, IErrorService, ProtoUtils, Timestamp} from 'jde-framework'
import { EAccess, ETypes } from '../../../model/types';
import {  MatTableModule } from '@angular/material/table';
import { Subscription } from 'rxjs';
import { ComponentPageTitle } from 'jde-spa';
import { MatToolbarModule } from '@angular/material/toolbar';
import { NodePageData } from '../../../services/resolvers/node.resolver';
import { OpcNodeRouteService } from '../../../services/routes/opc-node-route.service';
import { Value, valueString } from '../../../model/Value';
import { ENodeClass, Variable, UaNode }  from '../../../model/Node';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatDatepickerInputEvent, MatDatepickerModule } from '@angular/material/datepicker';
import { MatInputModule } from '@angular/material/input';
import {provideNativeDateAdapter} from '@angular/material/core';
import { Server } from '../../../model/Server';
import { NodeId } from '../../../model/NodeId';

@Component({
  selector: 'node-children',
  templateUrl: './node-children.html',
  styleUrls: ['./node-children.scss'],
  providers: [provideNativeDateAdapter()],
  standalone: true,
  imports: [RouterModule,MatButtonModule,MatCheckboxModule,MatDatepickerModule,MatFormFieldModule, MatInputModule,MatTableModule,MatToolbarModule, MatSelectModule]
})
export class NodeChildren implements OnInit, OnDestroy {
	constructor(
		@Inject('GatewayService') private gatewayService:GatewayService,
		private route: ActivatedRoute,
		@Inject('IErrorService') private snackbar: IErrorService,
		private componentPageTitle:ComponentPageTitle,
		private cdRef:ChangeDetectorRef)
	{}

	async ngOnInit() {
		this.route.data.subscribe( async (data)=>{
			if( this.pageData )
				this.#profileStore.save<UserSettings>( this.pageData.route.profileKey, this.profile );
			this.pageData = data["pageData"];
			this.profile = await this.#profileStore.load<UserSettings>( this.pageData.route.profileKey, new UserSettings() );
			this.profile.subscriptions = (this.profile.subscriptions ?? []).map( (s)=>new NodeId(s) );//revive persisted plain objects into NodeId instances (s.equals/s.key below would otherwise throw)
			this.nodes?.filter( (n:UaNode)=>this.profile.subscriptions.some((s)=>s.key==n.key) ).forEach( (n)=>this.selections.select(n) );
			this.isLoading.set( false );
			this.componentPageTitle.title = this.server.connection.name + (this.node().id==85 ? '' : `/${this.node.name}`);
		});
		this.selections.changed.subscribe( this.onSubscriptionChange.bind(this) );
	}

  ngOnDestroy() {
		this.selections.clear();
		this.selections.changed.unsubscribe();
		this.subscription = undefined;
  }

	async retrieveSnapshot(){
		this.retrievingSnapshot.set( true );
		this.variables.forEach( r=>r.value=undefined );
		var snapshots = await this._iot.snapshot( this.cnnctnTarget, this.variables );
		for( let [node,value] of snapshots ){
			let variable = this.variables.find( (n)=>n.equals(node) );
			if( variable )
				variable.value = value;
		}
		this.retrievingSnapshot.set( false );
	}

	toDate( value:Timestamp ):Date{
		return DateUtils.asUtc( ProtoUtils.toDate(value)! );
	}

	toObject( x:ENodeClass ):string{ return ENodeClass[x]; }
	toString( value:Value ){ return valueString(value); }
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
							this.variables.find( (r)=>r.nodeId.equals(value.node) )!.value = value.value;
						},
						error:(e: Error) =>{
							this.snackbar.exception( e, (m)=>console.log(m) );
						},
						complete:()=>{ console.debug( "complete" );}
					});
				}
				else
					this._iot.addToSubscription( this.cnnctnTarget, nodes, this.Key );
			} catch (e:any) {
				this.snackbar.error( e["error"], (m)=>console.log(m) );
			}
		}
		if( r.removed.length>0 ){
			let nodes = r.removed.map( r=>r.nodeId );
			this.profile.subscriptions = this.profile.subscriptions.filter( s=>!nodes.some(n=>n.key==s.key) );//compare by value: r.removed nodeIds are fresh instances, so reference includes() never matched
			if( !this.selections.selected.length )
				this.subscription = undefined;
			else{
				try{
					this._iot.unsubscribe( this.cnnctnTarget, nodes, this.Key );
				}
				catch( e:any ) {
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

	async toggleValue( x:Variable, e:MatCheckboxChange ){
		e.source.checked = !e.source.checked;
		try {
			x.value = await this._iot.write( this.cnnctnTarget, x.nodeId, !x.value, (x)=>console.log(x) );
			this.cdRef.detectChanges();
		}
		catch (e) {
			this.snackbar.exception( e, (m)=>console.log(m) );
		}
	}
	async changeDouble( x:Variable, e:Event ){
		try {
			x.value = await this._iot.write( this.cnnctnTarget, x.nodeId, +(e.target as any)["value"], (x)=>console.log(x) );
		}
		catch (e:any) {
			this.snackbar.exception( e, (m)=>console.log(m) );
			x.value = await this._iot.read( this.cnnctnTarget, x.nodeId );
			console.log(x.value);
		}
	}
	async changeString( n:Variable, e:Event ){
		try{
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, (e.target as any)["value"], (x)=>console.log(x) );
		}
		catch(err:any){
			(e.target as any)["value"] = n.value;
			this.snackbar.exception( err, (m)=>console.error(m) );
		}
	}
	async changeEnum( n:Variable, e:MatSelectChange<number> ){
		try{
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, e.value, (x)=>console.log(x) );
		}
		catch(err:any){
			e.source.value = <number>n.value;
			this.snackbar.exception( err, (m)=>console.error(m) );
		}
	}
	async dateInput( n:Variable, e:MatDatepickerInputEvent<Date, any> ){
		try {
			let date = DateUtils.beginningOfDay( e.value );
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, <Timestamp>ProtoUtils.fromDate(date), (x)=>console.log(x) );
		}
		catch (err) {
			e.target["value"] = n.value;
			this.snackbar.exception( err, (m)=>console.error(m) );
		}
	}
	async changeDate( n:Variable, e:Event ){
		try{
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, <Timestamp>ProtoUtils.fromDate(<Date>(e.target as any)["value"]), (x)=>console.log(x) );
		}
		catch(err:any){
			(e.target as any)["value"] = n.value;
			this.snackbar.exception( err, (m)=>console.error(m) );
		}
	}

	routerLink(n:UaNode):string[]{
		return [ `./${n.browseFQ(this.server.connection.defaultBrowseNs)}` ];
	}
	test(r:UaNode){}
	get columns():string[]{ return this.profile.columns; }
	EAccess = EAccess;
	ETypes = ETypes;
	get _iot():Gateway{ return this.pageData.gateway; }
	isAllSelected():boolean{ return this.selections.selected.length==this.nodes.length; }//was computed() over the non-signal SelectionModel — with no signal deps it cached its first value forever, freezing the header checkbox
	isLoading = signal<boolean>( true );
	get Key():string{ return this.pageData.route.profileKey; }
	node = model.required<UaNode>();
	get nodeId(){ return this.node().nodeId; }
	get server():Server{ return this.pageData.server; }
	get cnnctnTarget():string{ return this.server.connection.target; }
	pageData!:NodePageData;
	profile!:UserSettings;
	get nodes(){ return this.pageData?.nodes; }
	get variables():Variable[]{ return <Variable[]>this.nodes.filter((x)=>x.nodeClass==ENodeClass.Variable); }
	retrievingSnapshot = signal<boolean>( false );
	routerSubscription!:Subscription;
	selections = new SelectionModel<UaNode>(true, []);
	get showSnapshot():boolean{ return this.visibleColumns.includes("snapshot");}
	//sideNav = model.required<NodeRoute>();
	get sort(){ return this.profile.sort; };
	get subscription(){return this.#subscription;} #subscription:Subscription|undefined;
	set subscription(x){ if(!x && this.subscription) this.subscription.unsubscribe(); this.#subscription=x; }
	get visibleColumns(){ return this.profile.visibleColumns; }

	#routeService = inject( OpcNodeRouteService );
	#profileStore = inject( ProfileStore );
}
class UserSettings{
	tabIndex:number = 0;
	sort:Sort = {active: "name", direction: "asc"};
	visibleColumns:string[] = ['select', 'id', 'name', 'snapshot', "description"];
	columns:string[] = ['select', 'id', 'name', 'snapshot', "description"];
	subscriptions:NodeId[] = [];
//	access:NodeAccessProfile = new NodeAccessProfile();
}