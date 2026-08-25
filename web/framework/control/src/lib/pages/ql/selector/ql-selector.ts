import { Component, OnInit, input, signal, model, effect, computed, inject, untracked, viewChild } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { SelectionModel } from '@angular/cdk/collections';
import { ProfileStore } from 'jde-spa';
import { SnackbarService } from '../../../shared/snackbar/snackbar-service';
import { IGraphQL } from '../../../services/graphql';
import { CollectionItem, ListRoute, QLListData, QLListResolver } from '../../../services/ql-list-resolver';
import { MetaObject } from '../../../model/ql/schema/meta-object';
import { Operator } from '../../../model/ql/view';
import { arraysEqual } from '../../../utils/utils';
import { QLList } from '../list/ql-list';

//A collection's ql-list in selector mode:  the toolbar, saved views and settings panel of the collection's own list page,
//with the rows checked against a caller-owned selection of ids.
@Component( {
	selector: 'ql-selector',
	template: `@if( listData() ){ <ql-list [listData]=listData() [selector]=true [collectionDisplay]=type() [(selections)]=rowSelections></ql-list> }`,
	host: {class:'main-content mat-drawer-container my-content'},
	imports: [QLList]
})
export class QLSelector implements OnInit{
	constructor(){
		//rows → ids.  The list only holds the current page of rows, so an id it does not show (a deleted identity, one past
		//the page size, one a view filter hides) can be neither checked nor unchecked and must survive untouched.  The
		//original order is kept so the owner's arraysEqual against the loaded ids reads "unchanged" until a box is toggled.
		effect( ()=>{
			const rows = this.rowSelections();
			const list = this.list();
			if( !rows || !list )
				return;
			const shown = new Set<number>( list.data().map( r=>r.id ) );
			const checked = rows.selected.map( r=>r.id );
			const prior = untracked( ()=>this.selections().selected );//outside the tracking scope, or the set() below re-runs this forever
			const ids = [...prior.filter( id=>!shown.has(id) || checked.includes(id) ), ...checked.filter( id=>!prior.includes(id) )];
			if( !arraysEqual(ids, prior) )
				this.selections.set( new SelectionModel<number>(true, ids) );
		});
	}

	async ngOnInit(){
		try{
			//the collection's columns/sort/exclusions sit on the sibling ':collectionDisplay' route, where DetailResolver finds them too
			const collections:CollectionItem[] = this.route.snapshot.parent?.routeConfig?.children?.find( x=>x.path==":collectionDisplay" )?.data?.["collections"] ?? [];
			const collectionName = this.collectionName();
			let data = await QLListResolver.data( this.ql(), ListRoute.find(collectionName, collections), this.profileStore );
			data.profile.showDeleted = false;//a deleted identity is not something to add to a group; the list hides the toggle in selector mode
			if( this.excludedIds().length )
				data.fixedFilters = [{ field: data.schema.fields.find(f=>f.name=="id")!, filter: {operator: Operator.NotIn, value: this.excludedIds()} }];
			data = await QLListResolver.load( this.ql(), data, null );
			const ids = this.selections().selected;
			this.rowSelections.set( new SelectionModel<any>(true, data.results[collectionName].filter( (r:any)=>ids.includes(r.id) )) );
			this.listData.set( data );
		}
		catch( e ){
			this.snackbar.exception( "Could not load values", e );
		}
	};

	type = input.required<string>();
	ql = input.required<IGraphQL>();
	excludedIds = input<number[]>( [] );
	selections = model.required<SelectionModel<number>>();

	collectionName = computed<string>( ()=> MetaObject.toCollectionName(this.type()) );
	listData = signal<QLListData|undefined>( undefined );
	rowSelections = signal<SelectionModel<any>>( null as any );
	list = viewChild( QLList );

	private route = inject( ActivatedRoute );
	private profileStore = inject( ProfileStore );
	private snackbar = inject( SnackbarService );
}
