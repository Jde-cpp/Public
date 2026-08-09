//https://github.com/angular/components/blob/a55b19797f0bccf467d5602f526eef236737498b/docs/src/app/shared/navbar/navbar.ts
import { AsyncPipe } from '@angular/common';
import {Component, computed, inject, OnInit, signal} from '@angular/core';
import {FormControl, FormsModule, ReactiveFormsModule} from '@angular/forms';
import {MatAutocompleteModule} from '@angular/material/autocomplete';
import {MatButtonModule} from '@angular/material/button';
import {MatFormFieldModule} from '@angular/material/form-field';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatMenuModule } from '@angular/material/menu';
import {ActivatedRoute, NavigationEnd, RouterLink, RouterLinkActive} from '@angular/router';
import {Route, Router} from '@angular/router';
import { Title } from '@angular/platform-browser';
import { BehaviorSubject, filter, Observable } from 'rxjs';
import {NavigationFocusService} from '../navigation-focus/navigation-focus.service';
import {ThemePicker} from '../theme-picker/theme-picker';
import { Authorization } from '../authorization/authorization';
import { Favorites } from './favorites/favorites-dialog';
import { ProfileStore } from '../../services/profile/profile-store';
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
  imports: [
		AsyncPipe,
    Authorization,
    Favorites,
		FormsModule,
		MatAutocompleteModule,
    MatButtonModule,
		MatFormFieldModule,
    MatIconModule,
		MatInputModule,
    MatMenuModule,
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
			this.router.config.find( config=>{
				let configSegments = config.path!.split('/');
				let urlSegments = e.urlAfterRedirects.split('?')[0].split('/');
				urlSegments.shift();
				if( configSegments.length!=urlSegments.length )
					return false;
				for( let i=0; i<configSegments.length; i++ ){
					if( configSegments[i].startsWith(':') )
						continue;
					if( configSegments[i]!=urlSegments[i] )
						return false;
				}
				let title = config.title as string;
				this.name.set( !title || title.startsWith(':') ? e.urlAfterRedirects.split('?')[0].split('/').pop()! : title );
				return true;
			});
			this.route.set( e.urlAfterRedirects.split('?')[0] );
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
	defaultFavorites:Favorite[];
	favorites = signal<Favorite[]>(null as any);
	isLoading = signal<boolean>( true );
	name = signal<string>( null as any );
	route = signal<string>( null as any );
	existing = computed<Favorite|undefined>( ()=>this.favorites()?.find( fav=>fav.route==this.route() ) );// the favorite corresponding to the current route, if any
	router = inject(Router);
	searchForm = new FormControl<string>('');
}