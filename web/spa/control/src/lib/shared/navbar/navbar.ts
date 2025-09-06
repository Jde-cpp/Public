import {Component, OnDestroy} from '@angular/core';
import {MatButtonModule} from '@angular/material/button';
import {RouterLink, RouterLinkActive} from '@angular/router';
import {Route, Router, RouterModule} from '@angular/router';
import {SECTIONS} from '../documentation-items/documentation-items';
import {Subscription} from 'rxjs';
import {NavigationFocusService} from '../navigation-focus/navigation-focus.service';
import {ThemePicker} from '../theme-picker/theme-picker';
import { Authorization } from '../authorization/authorization';
const SECTIONS_KEYS = Object.keys(SECTIONS);

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
  ],
})
export class NavBar implements OnDestroy {
  private subscriptions = new Subscription();
  //isNextVersion = location.hostname === 'next.material.angular.io';
  skipLinkHref: string | null | undefined;
  skipLinkHidden = true;
  routes:Route[] = [];
  constructor(private navigationFocusService: NavigationFocusService, public router: Router) {
    this.routes = router.config.filter( x=>
			x.path!="login"
			&& x.path.indexOf(':target')==-1
			&& !x.path.includes('/')
			&& ( !x.children || x.children.find( y=>!y.path.length) )
		);
		let dflt = this.routes.find( x=> x.component && x.component.name==router.config.find( x=>!x.path.length )?.component.name );
		if( dflt )
			dflt["default"] = true;

    setTimeout(() => this.skipLinkHref = this.navigationFocusService.getSkipLinkHref(), 100);
  }

  get sections() {
    return SECTIONS;
  }

  get sectionKeys() {
    return SECTIONS_KEYS;
  }

  routerLinkOptions( route:Route ):{exact:boolean}{
    return {exact:!route.path.length};
  }

  name( route:Route ):string{
    return route.title?.valueOf() as string;
  }
  ngOnDestroy() {
    this.subscriptions.unsubscribe();
  }
}