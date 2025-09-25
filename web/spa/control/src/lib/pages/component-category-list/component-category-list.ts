import {Location} from '@angular/common';
import {Component, effect, model, NgModule, OnInit, Signal, signal } from '@angular/core';
import {MatCardModule} from '@angular/material/card';
import {ActivatedRoute, Params, Router, RouterModule, RouterLink} from '@angular/router';
import {MatRipple} from '@angular/material/core';
import {Observable, Subscription} from 'rxjs';
import {NavigationFocus} from '../../shared/navigation-focus/navigation-focus';
import {ComponentPageTitle} from '../page-title/page-title';
import {DocItem} from '../component-sidenav/component-sidenav'

@Component({
  selector: 'app-component-category-list',
  templateUrl: './component-category-list.html',
  styleUrls: ['./component-category-list.scss'],
  imports: [NavigationFocus, RouterLink, /*AsyncPipe,*/ MatRipple]
})
export class ComponentCategoryList implements OnInit/*, OnDestroy*/ {
  params: Observable<Params> | undefined;
  routeParamSubscription: Subscription = new Subscription();
  _categoryListSummary: string | undefined;
	items = model<Signal<DocItem[]>>();  // Needs to be model to be set from ComponentSidenav.onRouterOutletActivate
	isLoading = signal<boolean>( true );
	section = model<string>();
  constructor(/*public docItems: DocumentationItems,*/
    private router: Router,
    public _componentPageTitle: ComponentPageTitle,
    private route: ActivatedRoute,
    private _location:Location){
		if( !this.section )
    	this.section.set( (this._location.path().length ? '/' : '' )+this._location.path() );
		effect(() => {
			this.isLoading.set( this.items()()==null );
		});
  }

  ngOnInit(){
		this.route.title.subscribe( t=>{
			if( !t.startsWith(":") )
				this._componentPageTitle.title = t as string;
		} );
		this.route.paramMap.subscribe( p=>{
			if( this.route.snapshot.title.startsWith(":") )
				this._componentPageTitle.title = p.get( this.route.snapshot.title.substring(1) );
		} );
		const section = { name: this.route.data["value"].name, summary: this.route.data["value"].summary }; //
		this._categoryListSummary = section.summary; //
  }
	canActivate( component:DocItem ):boolean {
		return false;
	}
}

@NgModule({
  imports: [MatCardModule, RouterModule, ComponentCategoryList],
  exports: [ComponentCategoryList],
})
export class ComponentCategoryListModule { }