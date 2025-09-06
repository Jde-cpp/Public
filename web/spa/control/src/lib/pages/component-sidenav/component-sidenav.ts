import {
  Component,
  NgModule,
  NgZone,
  OnDestroy,
  OnInit,
  Signal,
  ViewEncapsulation,
  effect,
  forwardRef,
  input,
  model,
  signal,
  viewChild
} from '@angular/core';
import {animate, state, style, transition, trigger} from '@angular/animations';
import {CdkAccordionModule} from '@angular/cdk/accordion';
import {BreakpointObserver} from '@angular/cdk/layout';
import {AsyncPipe, NgClass} from '@angular/common';
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

import {
  ComponentCategoryListModule
} from '../component-category-list/component-category-list';
import {ComponentPageHeader} from '../component-page-header/component-page-header';
import { IRouteService, RouteService } from '../../services/IRouteService';

// These constants are used by the ComponentSidenav for orchestrating the MatSidenav in a responsive
// way. This includes hiding the sidenav, defaulting it to open, changing the mode from over to
// side, determining the size of the top gap, and whether the sidenav is fixed in the viewport.
// The values were determined through the combination of Material Design breakpoints and careful
// testing of the application across a range of common device widths (360px+).
// These breakpoint values need to stay in sync with the related Sass variables in
// src/styles/_constants.scss.
const EXTRA_SMALL_WIDTH_BREAKPOINT = 720;
const SMALL_WIDTH_BREAKPOINT = 959;
export class DocItem{
	constructor( args?:Partial<DocItem>){
		if( args )
			Object.assign( this, args );
	}
	get path(){ return this._path; } set path(x){ this._path=x; } private _path: string; //routerLink - access/groups or relative
	get title(){ return this._title; } set title(x){ this._title=x; } private _title: string;
	get queryParams(){ return this._queryParams; } set queryParams(x){ this._queryParams=x; } private _queryParams: Params;
	summary?: string;
	parent?:DocItem;
	siblings?:DocItem[]; //includes this.
	get track(){ return this.queryParams ? this.path+JSON.stringify(this.queryParams) : this.path; }
	excludedColumns?:string[];
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
  isExtraScreenSmall: Observable<boolean>;
  isScreenSmall: Observable<boolean>;
  private subscriptions = new Subscription();
	item = model<DocItem>(null);
  constructor( private _route: ActivatedRoute,
              private _navigationFocusService: NavigationFocusService,
              zone: NgZone,
              breakpoints: BreakpointObserver,
							private router: Router/*,
							@Optional() @Inject('IRouteService') private routeService:IRouteService*/) {
    this.isExtraScreenSmall =
        breakpoints.observe(`(max-width: ${EXTRA_SMALL_WIDTH_BREAKPOINT}px)`)
            .pipe(map(breakpoint => breakpoint.matches));
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
			debugger;
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
	styles: [`.child-nav { margin-left: 15px; }`],
  animations: [
    trigger('bodyExpansion', [
      state('collapsed', style({ height: '0px', display: 'none' })),
      state('expanded', style({ height: '*', display: 'block' })),
      transition('expanded <=> collapsed', animate('225ms cubic-bezier(0.4,0.0,0.2,1)')),
    ]),
  ],
  imports: [ MatIconModule, MatListModule, NgClass, RouterLinkActive, RouterLink ],
})
export class ComponentNav {
  constructor(private router: Router, private route: ActivatedRoute ){
		effect(() => {
			let loaded = this.item()()!=null;
			if( loaded ){
				let segments = this.parentUrl.split( "/" );
				if( segments[segments.length-1].startsWith(":") ){
					this.parentUrl = `${segments.slice(0,segments.length-1).join("/")}/${this.item()().parent?.path}`;
				}
			}
			this.isLoading.set( !loaded );
		});
	}
  ngOnInit(){
		this.route.url.subscribe( (urlSegments) => {
			this.parentUrl = urlSegments.slice(0, -1).map(segment => segment.path).join('/');
		});
 	};
  isRoot( url:string ){
    return url==`/${this.parentUrl}` || url.substr( this.parentUrl.length+2 ).indexOf('/')!=-1;
  }
  currentItemId: string | undefined;
	item = input.required<Signal<DocItem>>();
	parentUrl: string;
	isLoading = signal( true );
}

@NgModule({
  imports: [
    MatSidenavModule,
    MatListModule,
    RouterModule,
    ComponentCategoryListModule,
    FormsModule,
    CdkAccordionModule,
    MatIconModule,
    ComponentSidenav,
    ComponentNav
  ],
  exports: [ComponentSidenav],
})
export class ComponentSidenavModule {}
