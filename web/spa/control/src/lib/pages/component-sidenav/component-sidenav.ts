import {
  Component,
  NgModule,
  NgZone,
  OnDestroy,
  OnInit,
  ViewEncapsulation,
  effect,
  forwardRef,
  input,
  model,
  signal,
  viewChild
} from '@angular/core';
import {CdkAccordionModule} from '@angular/cdk/accordion';
import {BreakpointObserver} from '@angular/cdk/layout';
import {AsyncPipe} from '@angular/common';
import {FormsModule} from '@angular/forms';
import {MatIconModule} from '@angular/material/icon';
import {MatListModule} from '@angular/material/list';
import {MatSidenav, MatSidenavModule} from '@angular/material/sidenav';
import {
  ActivatedRoute,
  Params,
  RouterModule,
  Router,
  RouterOutlet,
  RouterLinkActive,
  RouterLink,
} from '@angular/router';
import {combineLatest, Observable, Subscription} from 'rxjs';
import {map} from 'rxjs/operators';

import {
  NavigationFocusService
} from '../../shared/navigation-focus/navigation-focus.service';
import {ComponentPageHeader} from '../component-page-header/component-page-header';
import { ComponentCategoryList } from '../component-category-list/component-category-list';

// Used by the ComponentSidenav for orchestrating the MatSidenav in a responsive way: hiding the
// sidenav, defaulting it to open, and changing the mode from over to side.
// The value was determined through the combination of Material Design breakpoints and careful
// testing of the application across a range of common device widths (360px+).
// It needs to stay in sync with the related Sass variables in src/styles/_constants.scss.
const SMALL_WIDTH_BREAKPOINT = 959;
export class RouteItem{
	constructor( args?:Partial<RouteItem>){
		if( args )
			Object.assign( this, args );
	}
	cardClass?: string;
	externalRedirect?: string;
	icon?: string;
	parent?:RouteItem;
	get path(){ return this._path; } set path(x){ this._path=x; } private _path!: string; //routerLink - access/groups or relative
	get queryParams(){ return this._queryParams; } set queryParams(x){ this._queryParams=x; } private _queryParams!: Params;
	siblings?:RouteItem[]; //includes this.
	summary?: string;
	get title(){ return this._title; } set title(x){ this._title=x; } private _title!: string;
	get track(){ return this.queryParams ? this.path+JSON.stringify(this.queryParams) : this.path; }
}

// Sidebar + router_outlet
@Component({
  selector: 'app-component-sidenav',
  templateUrl: './component-sidenav.html',
  styleUrls: ['./component-sidenav.scss'],
  encapsulation: ViewEncapsulation.None,
  imports: [ MatSidenavModule, forwardRef(() => ComponentNav), ComponentPageHeader, RouterOutlet, AsyncPipe ],
})
export class ComponentSidenav implements OnInit, OnDestroy {
  readonly sidenav = viewChild(MatSidenav);
  params: Observable<Params> | undefined;
  isScreenSmall: Observable<boolean>;
  private subscriptions = new Subscription();
	item = model<RouteItem>(null as any);
  constructor( private _route: ActivatedRoute,
              private _navigationFocusService: NavigationFocusService,
              zone: NgZone,
              breakpoints: BreakpointObserver,
							private router: Router/*,
							@Optional() @Inject('IRouteService') private routeService:IRouteService*/) {
    this.isScreenSmall = breakpoints.observe(`(max-width: ${SMALL_WIDTH_BREAKPOINT}px)`).pipe(map(breakpoint => breakpoint.matches));
  }

  ngOnInit() {
    // Combine params from all of the path into a single object.
    this.params = combineLatest(
        this._route.pathFromRoot.map(route => route.params), Object.assign);

    this.subscriptions.add(
      this._navigationFocusService.navigationEndEvents.pipe(map(() => this.isScreenSmall))
      .subscribe((shouldCloseSideNav) => {
          const sidenav = this.sidenav();
          if (shouldCloseSideNav && sidenav) {
            sidenav.close();
          }
        }
    ));
  }
  onRouterOutletActivate( event : any ){//
		if( 'sideNav' in event ){
			event.sideNav = this.item;
		}
		else
			console.warn( `onRouterOutletActivate: activated component has no 'sideNav' input` );
	}
  ngOnDestroy() {
    this.subscriptions.unsubscribe();
  }

  toggleSidenav(): void {
    this.sidenav()?.toggle();
  }
}

@Component({
  selector: 'app-component-nav',
  templateUrl: './component-nav.html',
	styles: [`
		.parent-nav {
			--mat-list-list-item-one-line-container-height: 32px;
			--mat-list-list-item-label-text-size: .75rem;
			--mat-list-list-item-label-text-weight: 500;
			--mat-list-list-item-label-text-tracking: .08em;
			text-transform: uppercase;
			opacity: .7;
			margin-bottom: 4px;
		}
	`],
  imports: [ MatIconModule, MatListModule, RouterLinkActive, RouterLink ],
})
export class ComponentNav {
  constructor(private router: Router, private route: ActivatedRoute ){
		effect(() => {
			let loaded = this.item()!=null;
			if( loaded ){
				if( this.item().parent )
					this.parentUrl = this.item().parent!.path;
				else{
					let segments = this.parentUrl.split( "/" );
					if( segments[segments.length-1].startsWith(":") )
						this.parentUrl = `${segments.slice(0,segments.length-1).join("/")}/${this.item().parent?.path}`;
					//else
					//	this.parentUrl = '';
				}
			}
			this.isLoading.set( !loaded );
		});
	}
  ngOnInit(){
		this.route.url.subscribe( (urlSegments) => {
			if( urlSegments.length==1 )
				this.parentUrl = '/'+urlSegments[0].path; //access/users
			else
				this.parentUrl = '/'+urlSegments.slice(0, -1).map(segment => segment.path).join('/');
		});
 	};
  isRoot( url:string ){
    return url==`/${this.parentUrl}` || url.substr( this.parentUrl.length+2 ).indexOf('/')!=-1;
  }
  currentItemId: string | undefined;
	item = input.required<RouteItem>();
	parentUrl!: string;
	isLoading = signal( true );
}

@NgModule({
  imports: [
    MatSidenavModule,
    MatListModule,
    RouterModule,
    ComponentCategoryList,
    FormsModule,
    CdkAccordionModule,
    MatIconModule,
    ComponentSidenav,
    ComponentNav
  ],
  exports: [ComponentSidenav],
})
export class ComponentSidenavModule {}
