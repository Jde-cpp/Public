import { SelectionModel, SelectionChange } from '@angular/cdk/collections';
import {ChangeDetectorRef, Component, computed, inject, Inject, model, OnDestroy, OnInit, signal} from '@angular/core';
import {MatButtonModule} from '@angular/material/button';
import {MatButtonToggleModule} from '@angular/material/button-toggle';
import {MatCheckboxChange, MatCheckboxModule} from '@angular/material/checkbox';
import {MatIconModule} from '@angular/material/icon';
import {MatProgressBarModule} from '@angular/material/progress-bar';
import {MatSelectChange, MatSelectModule} from '@angular/material/select';
import { MatSortModule, Sort } from "@angular/material/sort";
import {MatTooltipModule} from '@angular/material/tooltip';
import {RouterModule, ActivatedRoute, Router} from '@angular/router';
import { Gateway, GatewayService, SubscriptionResult } from '../../../services/gateway-service';
import { ProfileStore } from 'jde-spa';
import { DateUtils, QLListSettings, SnackbarService, ProtoUtils, Timestamp, View, ViewType} from 'jde-framework'
import { EAccess, ETypes } from '../../../model/types';
import {  MatTableModule } from '@angular/material/table';
import { Subscription } from 'rxjs';
import { ComponentPageTitle } from 'jde-spa';
import { MatToolbarModule } from '@angular/material/toolbar';
import { NodePageData } from '../../../services/resolvers/node-resolver';
import { OpcNodeRouteService } from '../../../services/routes/opc-node-route-service';
import { Value, valueString } from '../../../model/value';
import { ENodeClass, Variable, UaNode }  from '../../../model/node';
import { NodeView } from '../../../model/node-view';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatDatepickerInputEvent, MatDatepickerModule } from '@angular/material/datepicker';
import { MatInputModule } from '@angular/material/input';
import {provideNativeDateAdapter} from '@angular/material/core';
import { Server } from '../../../model/server';
import { NodeId } from '../../../model/node-id';

@Component({
  selector: 'node-children',
  templateUrl: './node-children.html',
  styleUrls: ['./node-children.scss'],
  providers: [provideNativeDateAdapter()],
  imports: [RouterModule,MatButtonModule,MatButtonToggleModule,MatCheckboxModule,MatDatepickerModule,MatFormFieldModule,MatIconModule,MatInputModule,MatProgressBarModule,MatSortModule,MatTableModule,MatToolbarModule,MatTooltipModule,MatSelectModule,QLListSettings]
})
export class NodeChildren implements OnInit, OnDestroy {
	constructor(
		@Inject('GatewayService') private gatewayService:GatewayService,
		private route: ActivatedRoute,
		private snackbar: SnackbarService,
		private componentPageTitle:ComponentPageTitle,
		private cdRef:ChangeDetectorRef)
	{}

	async ngOnInit() {
		//the views are the table's, not a node's:  loaded once, ahead of the first node.  route.data replays its current
		//value on subscribe, so a navigation during the load is not missed.
		const saved = await this.#profileStore.loadClassArray<NodeView>( `${NodeView.collectionName}/views`, NodeView, NodeView.schema );
		this.views.set( [NodeView.default(), ...saved] );
		this.viewIndex.set( Math.min(ProfileStore.viewIndex(NodeView.collectionName), this.views().length-1) );
		this.route.data.subscribe( async (data)=>{
			if( this.pageData )
				this.#profileStore.save<UserSettings>( this.pageData.route.profileKey, this.profile );
			this.pageData = data["pageData"];
			//A node switch is a param change, so Angular reuses this component and ngOnDestroy never runs: without this the
			//previous node's rows stayed selected, and since `nodes` is a fresh UaNode array per navigation, re-selecting the
			//persisted subscriptions below added a SECOND entry for the same node rather than being a no-op (SelectionModel
			//dedupes by object identity).  Three round trips left four copies of one node, and isAllSelected() compares
			//`selected.length` to `nodes.length`, so the header checkbox went wrong.
			//A NEW model rather than selections.clear(): clear() emits `removed`, which onSubscriptionChange cannot tell from
			//the user unticking the rows - it would unsubscribe and drop the persisted subscriptions.  Dropping the live
			//subscription with it matches what ngOnDestroy does, and keeps it paired with the owner key, which is per node.
			this.subscription = undefined;
			this.profile = await this.#profileStore.load<UserSettings>( this.pageData.route.profileKey, new UserSettings() );
			this.profile.subscriptions = (this.profile.subscriptions ?? []).map( (s)=>new NodeId(s) );//revive persisted plain objects into NodeId instances (s.equals/s.key below would otherwise throw)
			this.setNodes( this.pageData.nodes, true );
			this.isLoading.set( false );
			this.componentPageTitle.title = this.server.connection.name + (this.node().id==85 ? '' : `/${this.node().name}`);
		});
	}

  ngOnDestroy() {
		this.selections.clear();
		this.selections.changed.unsubscribe();
		this.subscription = undefined;
  }

	//fresh UaNode objects - a navigation, or a re-browse of the same node.  The persisted subscriptions pick out the rows to
	//show ticked.  On a navigation the live subscription was dropped above, so select() them and let `added` recreate it at
	//the gateway;  a re-browse keeps the live subscription (it is keyed by NodeId), so the new model is built with them
	//pre-selected instead - the constructor does not emit `changed`, so nothing is re-subscribed, and nothing unsubscribed.
	private setNodes( nodes:UaNode[], resubscribe:boolean ){
		this.pageData.nodes = nodes;
		this.nodes.set( nodes );
		const persisted = nodes.filter( n=>this.profile.subscriptions.some(s=>s.key==n.key) );
		this.selections = new SelectionModel<UaNode>( true, resubscribe ? [] : persisted );
		this.selections.changed.subscribe( this.onSubscriptionChange.bind(this) );
		if( resubscribe )
			this.selections.select( ...persisted );
	}
	async onRefresh(){
		this.isRefreshing.set( true );
		try{
			const references = await this._iot.browseObjectsFolder( this.cnnctnTarget, this.node(), true, (m)=>console.log(m) );
			this.setNodes( references.filter( r=>r.displayed ), false );
		}
		catch( e ){
			this.snackbar.exception( "Could not refresh nodes.", e );
		}
		finally{
			this.isRefreshing.set( false );
		}
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

	//The view group and settings panel, as ql-list/log-detail have them.  A header sort or the panel's Show makes an Adhoc
	//twin of the current view (last in the group, "(edited)"), Save turns it into a User view and persists it;  the rows
	//re-derive from view() on their own.  The views are always replaced, never edited in place, so rows() sees the change.
	private setViews( views:NodeView[], index:number ){
		this.views.set( views );
		this.viewIndex.set( index );
		ProfileStore.setViewIndex( NodeView.collectionName, index );
	}
	private saveViews( views:NodeView[] ){
		this.#profileStore.save( `${NodeView.collectionName}/views`, views.filter(v=>v.isUser).map(v=>v.toJson(undefined)) )
			.catch( e=>this.snackbar.exception("Could not save view.", e) );
	}
	private static upsert( views:NodeView[], view:NodeView ):number{
		let index = views.findIndex( v=>v.name==view.name && v.type==view.type );
		if( index==-1 )
			index = views.push( view )-1;
		else
			views[index] = view;
		return index;
	}
	onSortChange( sort:Sort ){
		const view = this.viewCopy();
		view.sort = [sort, ...view.sort.filter( s=>s.active!=sort.active )];
		this.onViewShow( view );
	}
	onViewShow( shown:View ){//ql-list-settings emits a base View - re-wrap it so apply() is there
		this.isSettings.set( false );
		if( shown.name?.endsWith("*") && shown.isAdhoc )
			shown.name = shown.name.slice( 0, -1 );
		shown.type = ViewType.Adhoc;
		const views = [...this.views()];
		this.setViews( views, NodeChildren.upsert(views, new NodeView(shown)) );
	}
	onViewSave( saved:View ){
		const view = new NodeView( saved );
		if( !(view.isSystem && this.views().find(v=>v.name==view.name && v.isSystem)) )
			view.type = ViewType.User;
		const views = this.views().filter( v=>!v.isAdhoc );
		this.setViews( views, NodeChildren.upsert(views, view) );
		if( view.isUser )
			this.saveViews( views );
		this.isSettings.set( false );
	}
	onViewDelete( view:View ){
		if( view.isUser ){//the panel disables Delete for anything else;  this is the belt to those braces
			const current = this.view();
			const views = this.views().filter( v=>!(v.name==view.name && (v.isUser || v.isAdhoc)) );//and its unsaved twin
			this.setViews( views, Math.max(views.indexOf(current), 0) );
			this.saveViews( views );
		}
		this.isSettings.set( false );
	}
	onChangeView( index:number ){
		const selected = this.views()[index];
		if( !selected )
			return;
		const views = this.views().filter( v=>!v.isAdhoc || v===selected );//switching away discards the unsaved edit
		this.setViews( views, views.indexOf(selected) );
	}
	colSuggestions():Record<string,any[]>{
		const suggestions:Record<string,any[]> = {};
		for( const name of Object.keys(NodeView.columns) ){
			const values = this.nodes().map( n=>NodeView.cellValue(n, name) ).filter( v=>v!=null && v!=="" );
			suggestions[name] = [...new Set(values)].slice( 0, 100 ).sort( (a,b)=>typeof a=="number" && typeof b=="number" ? a-b : String(a).localeCompare(String(b)) );
		}
		return suggestions;
	}

	toDate( value:Timestamp|undefined ):Date|null{
		const date = value ? ProtoUtils.toDate( value ) : null;
		return date ? DateUtils.asUtc( date ) : null;
	}

	toObject( x:ENodeClass ):string{ return ENodeClass[x]; }
	toString( value:Value|undefined ){ return valueString(value); }//Variable.value is optional, and retrieveSnapshot clears it while reading
	dataType( n:UaNode ):string{ return NodeView.cellValue( n, "dataType" )?.toString() ?? ""; }
	access( n:UaNode ):string{ return NodeView.cellValue( n, "access" )?.toString() ?? ""; }
  checkboxLabel(row?: UaNode): string {
		return row
			? `${this.selections.isSelected(row) ? 'deselect' : 'select'} ${row.name}`
			: `${this.isAllSelected() ? 'select' : 'deselect'} all`;
  }
	async onSubscriptionChange( r:SelectionChange<UaNode> ){
		if( r.added.length>0 ){
			try {
				let nodes = r.added.map( r=>r.nodeId );
				//Dedupe by key.  Restoring persisted subscriptions re-selects those rows, and `selections.changed` is subscribed
				//before the awaited profile load resumes, so that restore re-enters here — a blind push re-added every
				//already-persisted NodeId on every visit and the array grew by N each time until an unsubscribe pruned the key.
				//Compare by key like the removal branch below: these are fresh NodeId instances, so identity never matches.
				const unsaved = nodes.filter( n=>!this.profile.subscriptions.some(s=>s.key==n.key) );
				this.profile.subscriptions.push( ...unsaved );//`nodes` (not `unsaved`) still drives the gateway calls below - after a reload the live subscription does not exist yet even for persisted keys
				if( !this.subscription){
					this.subscription = this._iot.subscribe( this.cnnctnTarget, nodes, this.Key ).subscribe({
						next:(value: SubscriptionResult) =>{
							this.variables.find( (r)=>r.nodeId.equals(value.node) )!.value = value.value;
						},
						error:(e: Error) =>{
							this.snackbar.exception( "Subscription error.", e );
						},
						complete:()=>{ console.debug( "complete" );}
					});
				}
				else
					this._iot.addToSubscription( this.cnnctnTarget, nodes, this.Key );
			} catch (e:any) {
				this.snackbar.exception( "Could not add subscription.", e );
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
					this.snackbar.exception( "Could not remove subscription.", e );
				}
			}
		}
	}

	//the header box covers the rows on screen that can be subscribed - an object has no value, and its own box is disabled
	get selectableRows():UaNode[]{ return this.rows().filter( n=>n.isVariable ); }
	isAllSelected():boolean{ const rows = this.selectableRows; return rows.length>0 && rows.every( n=>this.selections.isSelected(n) ); }//a plain method:  a computed() over the non-signal SelectionModel cached its first value forever
  toggleAllRows() {
		if( this.isAllSelected() )
			this.selections.deselect( ...this.selectableRows );
		else
			this.selections.select( ...this.selectableRows );
  }

	async toggleValue( x:Variable, e:MatCheckboxChange ){
		e.source.checked = !e.source.checked;
		try {
			x.value = await this._iot.write( this.cnnctnTarget, x.nodeId, !x.value, (x)=>console.log(x) );
			this.cdRef.detectChanges();
		}
		catch (e) {
			this.snackbar.exception( "Could not toggle value.", e );
		}
	}
	async changeDouble( x:Variable, e:Event ){
		try {
			x.value = await this._iot.write( this.cnnctnTarget, x.nodeId, +(e.target as any)["value"], (x)=>console.log(x) );
		}
		catch (e:any) {
			this.snackbar.exception( "Could not change double value.", e );
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
			this.snackbar.exception( "Could not change string value.", err );
		}
	}
	async changeEnum( n:Variable, e:MatSelectChange<number> ){
		try{
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, e.value, (x)=>console.log(x) );
		}
		catch(err:any){
			e.source.value = <number>n.value;
			this.snackbar.exception( "Could not change enum value.", err );
		}
	}
	async dateInput( n:Variable, e:MatDatepickerInputEvent<Date, any> ){
		try {
			let date = DateUtils.beginningOfDay( e.value );
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, <Timestamp>ProtoUtils.fromDate(date), (x)=>console.log(x) );
		}
		catch (err) {
			e.target["value"] = n.value;
			this.snackbar.exception( "Could not change date input.", err );
		}
	}
	async changeDate( n:Variable, e:Event ){
		try{
			n.value = await this._iot.write( this.cnnctnTarget, n.nodeId, <Timestamp>ProtoUtils.fromDate(<Date>(e.target as any)["value"]), (x)=>console.log(x) );
		}
		catch(err:any){
			(e.target as any)["value"] = n.value;
			this.snackbar.exception( "Could not change date value.", err );
		}
	}

	routerLink(n:UaNode):string[]{
		return [ `./${n.browseFQ(this.server.connection.defaultBrowseNs)}` ];
	}
	test(r:UaNode){}
	EAccess = EAccess;
	ETypes = ETypes;
	get _iot():Gateway{ return this.pageData.gateway; }
	isLoading = signal<boolean>( true );
	isRefreshing = signal<boolean>( false );
	isSettings = signal<boolean>( false );
	get Key():string{ return this.pageData.route.profileKey; }
	node = model.required<UaNode>();
	get nodeId(){ return this.node().nodeId; }
	get server():Server{ return this.pageData.server; }
	get cnnctnTarget():string{ return this.server.connection.target; }
	pageData!:NodePageData;
	profile!:UserSettings;
	nodes = signal<UaNode[]>( [] );//every child the browse returned;  rows() is the view's cut of them
	rows = computed<UaNode[]>( ()=>this.view() ? this.view().apply( this.nodes() ) : [] );
	displayedColumns = computed<string[]>( ()=>this.view()?.displayedColumns ?? [] );
	get variables():Variable[]{ return <Variable[]>this.nodes().filter((x)=>x.nodeClass==ENodeClass.Variable); }
	retrievingSnapshot = signal<boolean>( false );
	routerSubscription!:Subscription;
	selections = new SelectionModel<UaNode>(true, []);
	//sideNav = model.required<NodeRoute>();
	get sort():Sort{ return this.view().sort[0] ?? {active: "name", direction: "asc"}; }
	get subscription(){return this.#subscription;} #subscription:Subscription|undefined;
	set subscription(x){ if(!x && this.subscription) this.subscription.unsubscribe(); this.#subscription=x; }
	views = signal<NodeView[]>( [] );
	viewIndex = signal<number>( 0 );
	view = computed<NodeView>( ()=>this.views()[this.viewIndex()] );
	viewCopy = computed<NodeView>( ()=>new NodeView(this.view()) );//the settings panel gets a copy to edit, never the one on screen - and one copy per view, not one per change-detection pass as a getter would hand it
	columns = NodeView.columns;
	schema = NodeView.schema;

	#routeService = inject( OpcNodeRouteService );
	#profileStore = inject( ProfileStore );
}
//per node (keyed by profileKey):  the subscriptions and the tab.  The columns and sort used to live here too;  they are the view's now, shared by every node.
class UserSettings{
	tabIndex:number = 0;
	subscriptions:NodeId[] = [];
//	access:NodeAccessProfile = new NodeAccessProfile();
}
