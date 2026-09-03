import { Directive, effect, inject, model, OnDestroy, OnInit, signal } from '@angular/core';
import { ActivatedRoute, Router } from '@angular/router';
import { ComponentPageTitle, ProfileStore, RouteItem } from 'jde-spa';
import { SnackbarService } from '../shared/snackbar/snackbar-service';
import { DetailResolverData } from '../services/detail-resolver';
import { IGraphQL } from '../services/graphql';
import { TargetRow } from '../model/ql/target-row';

//The skeleton user-detail, role-detail, group-detail and client-detail each carried a copy of (review3 C2).  The copies
//had already drifted twice: L2's `$new` tab-index clamp existed in ONE of the four, and each page had reinvented the
//dirty-check effect, the route.data subscription and the save/cancel pair.  What is left in a subclass is what actually
//differs - the child collections it owns, and the row it builds to save.
//@Directive() rather than a bare class: Angular refuses to inherit `model()`/`input()` from an undecorated base.
@Directive()
export abstract class DetailPage<T extends TargetRow<T> & {properties:Partial<T>}> implements OnInit, OnDestroy{
	constructor( private readonly profileKey:string ){
		this.tabIndex.set( ProfileStore.tabIndex(profileKey) );
		effect( ()=>{
			const edited = this.properties();
			if( !edited )
				return;
			if( !edited.canSave )
				this.isChanged.set( false );
			else if( !(<T>edited).equals(<any>this.row.properties) )//`any`: ServerCnnctn.equals is declared over ITargetRow, the others over Partial<T>
				this.isChanged.set( true );
		});
	}

	//Subscribed HERE, not in the constructor:  a subclass's own fields - its SelectionModels - initialise only after
	//super() returns, and route.data emits synchronously, so onRow() would have run against fields that did not exist yet.
	ngOnInit(){
		this.route.data.subscribe( (data)=>this.#load(data) );
	}
	ngOnDestroy(){ ProfileStore.setTabIndex( this.profileKey, this.tabIndex() ); }
	onTabIndexChanged( index:number ){ this.tabIndex.set( index ); }

	#load( data:any ):void{
		this.pageData = data["pageData"];
		this.row = new (this.ctor)( this.pageData.row );
		if( this.onlyPropertiesTab && this.tabIndex()>0 )
			this.tabIndex.set( 0 );//review3 L2: every tab past Properties sits behind an @if a new record fails, and mat-tab-group hard-loops on an index it cannot resolve
		this.pageData.row = null;
		this.properties.set( this.row.properties );
		this.sideNav.set( this.pageData.routing );
		this.componentPageTitle.title = this.title;
		this.onRow();
		this.isLoading.set( false );
	}

	async onSubmitClick(){
		try{
			await this.ql.mutate( this.upsert().mutation(this.row), (m)=>console.log(m) );
			this.router.navigate( ['..'], {relativeTo: this.route} );
		}
		catch( e ){
			this.snackbar.exception( "Save failed.", e );
		}
	}
	onCancelClick(){ this.router.navigate( ['..'], {relativeTo: this.route} ); }

	protected abstract get ctor():new (item:any)=>T;//a constructor cannot be reached through T, and a subclass field would initialise too late for #load
	protected abstract upsert():T;//the edited row to save - the one thing every page assembles differently
	protected abstract onRow():void;//per-page state off the freshly loaded `row`
	protected get onlyPropertiesTab():boolean{ return !this.row.id; }//client-detail gates its extra tab on `server`, not on the id
	protected get title():string{ return this.row.name; }

	row!:T;
	pageData!:DetailResolverData<T>;
	get schema(){ return this.pageData.schema; }
	get id(){ return this.row.id; }
	isChanged = signal<boolean>( false );
	isLoading = signal<boolean>( true );
	properties = signal<Partial<T>>( null as any );
	sideNav = model.required<RouteItem>();
	tabIndex = signal<number>( 0 );
	abstract ql:IGraphQL;

	protected route = inject( ActivatedRoute );
	protected router = inject( Router );
	protected componentPageTitle = inject( ComponentPageTitle );
	protected snackbar = inject( SnackbarService );
}
