//https://github.com/angular/components/blob/a55b19797f0bccf467d5602f526eef236737498b/docs/src/app/shared/navbar/navbar.ts
import { AsyncPipe } from '@angular/common';
import {Component, computed, ElementRef, inject, OnInit, signal, viewChild} from '@angular/core';
import {FormControl, FormsModule, ReactiveFormsModule} from '@angular/forms';
import {MatAutocompleteModule} from '@angular/material/autocomplete';
import {MatButtonModule} from '@angular/material/button';
import {MatFormFieldModule} from '@angular/material/form-field';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatMenuModule } from '@angular/material/menu';
import { MatTooltipModule } from '@angular/material/tooltip';
import {ActivatedRoute, NavigationEnd, RouterLink, RouterLinkActive} from '@angular/router';
import {Route, Router, Routes} from '@angular/router';
import { Title } from '@angular/platform-browser';
import { BehaviorSubject, filter, Observable } from 'rxjs';
import {NavigationFocusService} from '../navigation-focus/navigation-focus-service';
import {ThemePicker} from '../theme-picker/theme-picker';
import { Authorization } from '../authorization/authorization';
import { Breadcrumbs } from './breadcrumbs/breadcrumbs';
import { Favorites } from './favorites/favorites-dialog';
import { ProfileStore } from '../../services/profile/profile-store';
import { RouteStore } from '../../services/route-store';
import { RouteItem } from '../../pages/component-sidenav/route-item';
import { MatAutocomplete } from "@angular/material/autocomplete";

export type Favorite={
	folderName?:string;
	name:string;
	route:string;
	queryParams?:any;
}
export type Folder = { folderName:string, items:Favorite[] };
@Component({
  selector: 'app-navbar',
  templateUrl: './navbar.html',
  styleUrls: ['./navbar.scss'],
  host: {
    '(document:keydown)': 'onKeydown($event)',
  },
  imports: [
		AsyncPipe,
    Authorization,
		Breadcrumbs,
    Favorites,
		FormsModule,
		MatAutocompleteModule,
    MatButtonModule,
		MatFormFieldModule,
    MatIconModule,
		MatInputModule,
    MatMenuModule,
		MatTooltipModule,
		ReactiveFormsModule,
    RouterLink,
    RouterLinkActive,
    ThemePicker
]
})
export class NavBar implements OnInit {
  skipLinkHref: string | null | undefined;
  skipLinkHidden = true;
  constructor( private navigationFocusService: NavigationFocusService ) {
    this.defaultFavorites = this.router.config.filter( x=>
			x.path!="login"
			&& x.path!.indexOf(':target')==-1
			&& !x.path!.includes('/')
			&& ( !x.children || x.children.find( y=>!y.path!.length) )
		).map( x=>({ name: x.title as string, route: '/'+x.path } ));
  }
	async ngOnInit(){
		this.router.events.pipe(//subscribe before the await:  the load defers the rest of ngOnInit past the initial NavigationEnd.
			filter( (e)=> e instanceof NavigationEnd )
		).subscribe( (e:NavigationEnd)=>{
			const path = e.urlAfterRedirects.split('?')[0];
			const crumbs = this.#buildCrumbs( path );
			this.crumbs.set( crumbs );
			this.name.set( crumbs[crumbs.length-1].title );
			this.route.set( path );
		});
		this.favorites.set( await this.#profileStore.load<Favorite[]>("favorites", this.defaultFavorites) );
		this.isLoading.set( false );
	}
	asFolder(item:Favorite|Folder):Folder{
		return item as Folder;
	}
  routerLinkOptions( route:Route ):{exact:boolean}{
    return {exact:!route.path!.length};
  }
	onFavoriteChange( change:Favorite ){
		const route = this.route();
		let favs;
		if( !change ) //delete
			favs = this.favorites().filter( fav=>fav.route!=route );
		else{
			favs = [ ...this.favorites() ];
			const index = favs.findIndex( fav=>fav.route==route );
			if( index==-1 ) //add
				favs.push( { ...change, route } );
			else //edit
				favs[index] = { ...favs[index], name: change.name, folderName: change.folderName };
		}
		this.favorites.set( favs );
		this.#profileStore.save( "favorites", favs );
	}
	onToggleBreadcrumbs(){
		const show = !this.showBreadcrumbs();
		this.showBreadcrumbs.set( show );
		this.#profileStore.save( "showBreadcrumbs", show );
	}
	#buildCrumbs( path:string ):RouteItem[]{
		const segments = path.split('/').filter( s=>s.length );
		const home = this.router.config.find( c=>c.path=='' );
		const crumbs = [ new RouteItem({ path: '/', title: (home?.title as string) ?? 'Home' }) ];
		for( let i=0; i<segments.length; i++ ){
			const config = NavBar.matchConfig( this.router.config, segments.slice(0, i+1) );
			let title = config?.title as string|undefined;
			if( !title || title.startsWith(':') )//no title, or the ':param' substitute-the-segment convention
				title = this.#segmentName( segments.slice(0,i).join('/'), segments[i] ) ?? decodeURIComponent( segments[i] );
			crumbs.push( new RouteItem({ path: config ? '/'+segments.slice(0,i+1).join('/') : undefined, title }) );//no matching route ⇒ no path ⇒ rendered as text, not a link
		}
		return crumbs;
	}
	#segmentName( parentUrl:string, segment:string ):string|undefined{//RouteStore writers key inconsistently: "gateways/gw1" (UrlSegments join), '/apps', bare "users"
		const last = parentUrl.split('/').pop() ?? '';
		for( const key of [parentUrl, '/'+parentUrl, last] ){
			const child = this.#routeStore.getChildren( key ).find( c=>c.path==segment || c.path?.endsWith('/'+segment) );//child paths are bare targets or parent-prefixed
			if( child?.title )
				return child.title;
		}
		return undefined;
	}
	static matchConfig( routes:Routes, segments:string[] ):Route|undefined{//first match wins ⇒ the duplicated 'access' path resolves like Angular's own matcher
		for( const config of routes ){
			if( config.path=='**' )
				return config;//wildcard consumes any remainder ⇒ every prefix inside a node browse path is routable
			const configSegments = config.path!.split('/').filter( s=>s.length );
			if( configSegments.length>segments.length || !configSegments.every( (cs,j)=>cs.startsWith(':') || cs==segments[j] ) )
				continue;
			if( configSegments.length==segments.length )
				return config;
			const child = config.children ? NavBar.matchConfig( config.children, segments.slice(configSegments.length) ) : undefined;
			if( child )
				return child;
		}
		return undefined;
	}

	onKeydown( event:KeyboardEvent ){//'/' jumps to search, unless the keystroke belongs to whatever the user is already typing in
		if( event.key!='/' || event.ctrlKey || event.metaKey || event.altKey || this.#isTyping(event.target) )
			return;
		event.preventDefault();//otherwise the '/' lands in the field we just focused
		this.searchInput()?.nativeElement.focus();
	}
	#isTyping( target:EventTarget|null ):boolean{
		const el = target as HTMLElement|null;
		return !!el && (el.isContentEditable || ['INPUT','TEXTAREA','SELECT'].includes(el.tagName));
	}
	onSearch( event:any ){

	}
	onSearchSelected( query:string ){
		//return [];
	}
	searchValuesSubject = new BehaviorSubject<string[]>([]);
	searchValues():Observable<string[]>{
		return this.searchValuesSubject.asObservable();
	}
	favoriteMenus = computed( ()=>{
		let items:Array<Favorite|Folder> = [];
		if( !this.favorites() )
			return [];
		for( let fav of this.favorites() ){
			if( !fav.folderName )
				items.push( fav );
			else{
				let select = items.find( x=>(x as Folder).folderName==fav.folderName ) as Folder;
				if( !select )
					items.push( {folderName: fav.folderName, items: [fav]} );
				else
					select.items.push( fav );
			}
		}
		return items;
	});
	#profileStore = inject(ProfileStore);
	#routeStore = inject(RouteStore);
	crumbs = signal<RouteItem[]>( [] );
	showBreadcrumbs = signal<boolean>( ProfileStore.local<boolean>("showBreadcrumbs", true) );//sync static read — awaiting #profileStore.load here would delay isLoading
	defaultFavorites:Favorite[];
	favorites = signal<Favorite[]>(null as any);
	isLoading = signal<boolean>( true );
	name = signal<string>( null as any );
	route = signal<string>( null as any );
	existing = computed<Favorite|undefined>( ()=>this.favorites()?.find( fav=>fav.route==this.route() ) );// the favorite corresponding to the current route, if any
	router = inject(Router);
	searchForm = new FormControl<string>('');
	searchInput = viewChild<ElementRef<HTMLInputElement>>( 'searchInput' );
}