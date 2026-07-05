import {inject, Inject, Injectable} from '@angular/core';
import {ActivatedRouteSnapshot, CanActivate, CanActivateChild, GuardResult, Router, RouterStateSnapshot, UrlTree} from '@angular/router';
import { AccessService } from 'jde-access';


@Injectable( {providedIn: 'root'} )
export class AuthGuard implements CanActivate, CanActivateChild {
	constructor( private router:Router)
	{}

	canActivate( route: ActivatedRouteSnapshot, state: RouterStateSnapshot ):Promise<GuardResult>{
		return Promise.resolve(true);
	}

	canActivateChild( childRoute: ActivatedRouteSnapshot, state: RouterStateSnapshot ):Promise<GuardResult>{
		return Promise.resolve(true);
	}
	accessService = inject(AccessService);
}
