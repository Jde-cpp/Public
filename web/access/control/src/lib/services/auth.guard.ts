import {Inject, Injectable} from '@angular/core';
import {ActivatedRouteSnapshot, CanActivate, CanActivateChild, GuardResult, Router, RouterStateSnapshot, UrlTree} from '@angular/router';


@Injectable( {providedIn: 'root'} )
export class AuthGuard implements CanActivate, CanActivateChild {
	constructor( @Inject('AccessService') private accessService, private router:Router)
	{}

	canActivate( route: ActivatedRouteSnapshot, state: RouterStateSnapshot ):Promise<GuardResult>{
		return Promise.resolve(true);
	}

	canActivateChild( childRoute: ActivatedRouteSnapshot, state: RouterStateSnapshot ):Promise<GuardResult>{
		return Promise.resolve(true);
	}
}
