//https://github.com/angular/components/blob/main/docs/src/app/pages/component-category-list/component-category-list.ts
import {Location, NgTemplateOutlet} from '@angular/common';
import {ChangeDetectionStrategy,Component, computed, effect, inject, model, NgModule, OnInit, Signal, signal } from '@angular/core';
import {MatCardModule} from '@angular/material/card';
import {ActivatedRoute, Params, Router, RouterModule, RouterLink} from '@angular/router';
//import {MatRipple} from '@angular/material/core';
import {combineLatest, Observable, Subscription} from 'rxjs';
import {NavigationFocus} from '../../shared/navigation-focus/navigation-focus';
import {ComponentPageTitle} from '../page-title/page-title';
import {RouteItem} from '../component-sidenav/component-sidenav'

@Component({
  selector: 'app-component-category-list',
  templateUrl: './component-category-list.html',
  styleUrls: ['./component-category-list.scss'],
  imports: [NavigationFocus, RouterLink, /*MatRipple,*/ NgTemplateOutlet],
	changeDetection: ChangeDetectionStrategy.Eager,
})
export class ComponentCategoryList implements OnInit/*, OnDestroy*/ {
  params: Observable<Params> | undefined;
  routeParamSubscription: Subscription = new Subscription();
  _categoryListSummary = model<string | undefined>();
  private readonly _componentPageTitle = inject(ComponentPageTitle);
	items = model<RouteItem[]>();  // Needs to be model to be set from ComponentSidenav.onRouterOutletActivate
	isLoading = computed<boolean>( () => this.items()==null );
	section = model<string>();
  private readonly _route = inject(ActivatedRoute);

  constructor(){
  }

  ngOnInit(){
    this.routeParamSubscription = combineLatest(
      this._route.pathFromRoot.map(route => route.params),
      Object.assign,
    ).subscribe( params=>{
			if( !params.data )
				return;
			const section = { name: params.data["value"].name, summary: params.data["value"].summary };
			if( params.title.startsWith(":") )
				this._componentPageTitle.title = params.title.substring(1);
			this._categoryListSummary.set( section.summary );
		});
  }
	ngOnDestroy(){
    if (this.routeParamSubscription) {
      this.routeParamSubscription.unsubscribe();
    }
  }
}