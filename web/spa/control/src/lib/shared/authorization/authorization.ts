import {Component, Inject, Signal, inject, resource} from '@angular/core';
import {Router, RouterLink, RouterLinkActive} from '@angular/router';
import {MatButtonModule} from '@angular/material/button';
import { EProvider, IAuth, User } from '../../services/authorization/IAuth';
import { MatIconModule } from '@angular/material/icon';

declare const gapi: any;
@Component( {
	selector: "authorization", templateUrl: "./authorization.html", styleUrls: ["./authorization.scss"],
	imports: [MatButtonModule, MatIconModule, RouterLink, RouterLinkActive]} )
export class Authorization{
	constructor( @Inject("IAuth") private authService: IAuth ){
		this.user = this.authService.user;
	}
  async onLogout() {
		let isGoogle = this.user().provider == EProvider.Google;
    await this.authService.logout( (m)=>console.log(m) );
		if( isGoogle && gapi.auth2 ){
			var auth2 = gapi.auth2.getAuthInstance();
			await auth2.signOut();
			console.log("User signed out.");
		}
		this.router.navigate( ['/login'] );
  }
	providers = resource<EProvider[], {}>({
    loader: async () => {
			try{
				let providers = await this.authService.providers( (m)=>console.log(m) );
				return providers;
			}
			catch( e ){
				console.error( e );
				return [];
			}
    }
	});

	router = inject(Router);
	user:Signal<User | null>;
}