//https://github.com/angular/components/blob/a55b19797f0bccf467d5602f526eef236737498b/docs/src/app/shared/navbar/navbar.ts
import {Component, computed, inject, OnInit, signal} from '@angular/core';
import {MatButtonModule} from '@angular/material/button';
import { MatMenuModule } from '@angular/material/menu';
import { MatIconModule } from '@angular/material/icon';
import {ActivatedRoute, NavigationEnd, RouterLink, RouterLinkActive} from '@angular/router';
import {Route, Router} from '@angular/router';
import { Title } from '@angular/platform-browser';
import { filter } from 'rxjs';
import {SECTIONS} from '../documentation-items/documentation-items';
import {NavigationFocusService} from '../navigation-focus/navigation-focus.service';
import {ThemePicker} from '../theme-picker/theme-picker';
import { Authorization } from '../authorization/authorization';
import { Favorites } from './favorites/favorites-dialog';
import { ProfileStore } from '../../services/profile-store';

const SECTIONS_KEYS = Object.keys(SECTIONS);

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
    Authorization,
    MatButtonModule,
    RouterLink,
    RouterLinkActive,
    ThemePicker,
    Favorites,
    MatMenuModule,
    MatIconModule,
  ]
})
export class NavBar implements OnInit {
  skipLinkHref: string | null | undefined;
  skipLinkHidden = true;
  constructor( private navigationFocusService: NavigationFocusService ) {
    this.defaultFavorites = this.router.config.filter( x=>
			x.path!="login"
			&& x.path.indexOf(':target')==-1
			&& !x.path.includes('/')
			&& ( !x.children || x.children.find( y=>!y.path.length) )
		).map( x=>({ name: x.title as string, route: '/'+x.path } ));
  }
	async ngOnInit(){
		this.favorites.set( this.defaultFavorites );
		//this.favorites.set( await this.#profileStore.load<Favorite[]>("favorites", this.defaultFavorites) );
		this.isLoading.set( false );

		this.router.events.pipe(
			filter( (e)=> e instanceof NavigationEnd )
		).subscribe( (e:NavigationEnd)=>{
			this.router.config.find( config=>{
				let configSegments = config.path.split('/');
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
				this.name.set( !title || title.startsWith(':') ? e.urlAfterRedirects.split('?')[0].split('/').pop() : title );
				this.route.set( e.urlAfterRedirects.split('?')[0] );
				return true;
			});
			let fav = this.favorites().find( fav=>{
				const y = fav.route==e.urlAfterRedirects.split('?')[0];
				return y ? fav : null;
			});
			this.existing.set( fav );
		})
	}
	asFolder(item:Favorite|Folder):Folder{
		return item as Folder;
	}
  routerLinkOptions( route:Route ):{exact:boolean}{
    return {exact:!route.path.length};
  }
	onFavoriteChange( change:Favorite ){
		let favs;
		if( this.existing() && !change ){ //delete
			favs = this.favorites().filter( fav=>fav.route!=this.existing().route );
			this.existing.set( null );
		}
		else{
			let newFav = { ...change, route: this.route() };
			let previous = this.favorites().find( fav=>fav.route==newFav.route );
			if( previous ){ //edit
				previous.name = newFav.name;
				previous.folderName = newFav.folderName;
				this.existing.set( previous );
				favs = [ ...this.favorites() ];
			}
			else{ //add
				favs = [ ...this.favorites(), newFav ];
				this.existing.set( newFav );
			}
		}
		this.favorites.set( favs );
		//this.#profileStore.save( "favorites", favs );
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
	favorites = signal<Favorite[]>(null);
	isLoading = signal<boolean>( true );
	name = signal<string>( null );
	route = signal<string>( null );
  get sections() {
    return SECTIONS;
  }

  get sectionKeys() {
    return SECTIONS_KEYS;
  }

	existing = signal<Favorite>( null );// the favorite corresponding to the current route, if any
	router = inject(Router);
}