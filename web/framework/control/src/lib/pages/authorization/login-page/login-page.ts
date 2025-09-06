import {Component, computed, effect, Inject, inject, Resource, resource} from '@angular/core';
import {Router, RouterLink} from "@angular/router";
import { MatButtonModule } from '@angular/material/button'
import { MatInputModule } from '@angular/material/input';
import {MatFormFieldModule} from '@angular/material/form-field';
import { EProvider, IAuth, IEnvironment, User } from 'jde-spa';
import {FormBuilder, ReactiveFormsModule} from "@angular/forms";
import {IErrorService} from '../../../services/error/IErrorService';

declare const google: any;

@Component({
    selector: 'app-login-page',templateUrl: './login-page.html',styleUrl: './login-page.scss',
    imports: [MatButtonModule,MatFormFieldModule,MatInputModule,RouterLink,ReactiveFormsModule],
})
export class LoginPageComponent{
	constructor( @Inject('IAuth') private authService: IAuth, @Inject('IErrorService') private snackbar: IErrorService, @Inject('IEnvironment') private envService: IEnvironment ){
		effect( async ()=>{
			if( this.providers?.value()?.includes(EProvider.Google) && !this.showedGoogleLogin ){
				this.showedGoogleLogin = true;
				if( !google )
					console.error( "google not defined" );
				else{
					let authId = await this.getGoogleAuthClientId();
					this.showGoogleLogin( authId );
				}
			}
		});

	}

  async onLogin() {
		let {username, password} = this.form.value;
		if (!username || !password) {
			this.snackbar.error( "Enter an email and password.", (m)=>console.log(m) );
			return;
		}
		let domain = null;
		if( username.indexOf('\\')!=-1 ){
			const parts = username.split('\\');
			domain = parts[0];
			username = parts[1];
		}
		try{
			await this.authService.loginPassword(
				domain,
				username,
				password,
				console.log
			);
			this.router.navigate( [''] );
		}
		catch( e ){
			this.snackbar.exception( e, (m)=>console.log(m) );
		}
  }
	onGoogleLogin2(credential:string){
		debugger;
		this.onGoogleLogin( credential );
	}
	async onGoogleLogin(credential: string){
		let user = new User( credential );
		try{
			await this.authService.login( user, console.log );
			localStorage.setItem( "googleLoginHint", user.email );
			this.router.navigate( [''] );
		}
		catch( e ){
			this.snackbar.exception( e, (m)=>console.log(m) );
		};
	}

	showGoogleLogin( authId:string ){
		let self = this;
		if( !google )
			throw "google not defined";
		try{
			google.accounts.id.initialize({
				client_id: authId,
				auto_select: true,
				button_auto_select: true,
				cancel_on_tap_outside: true,
				callback:  (response) => {self.onGoogleLogin(response.credential); },
				native_callback: (response) => {self.onGoogleLogin2(response.credential);},
				login_hint: localStorage.getItem( "googleLoginHint" )
			});
			google.accounts.id.renderButton(
				document.getElementById("google-button"),
				{ theme: "outline", size: "large" }
			);
			if( this.googleCredential && !this.user().picture )
				this.onGoogleLogin( this.googleCredential );
		}
		catch( e ){
			console.assert( !(e instanceof ReferenceError), "ReferenceError" );
			this.snackbar.exceptionInfo( e, 'could not render google login', (m)=>console.log(m) );
		}
	}

	async getGoogleAuthClientId():Promise<string>{
		const y = await this.authService.googleAuthClientId( console.log );
		if( !y )
			throw new Error( "googleAuthClientId is not defined" );
		return y;
	}
	hasUserPassword = computed(() => { return !this.providers.isLoading() && this.providers.value().includes( EProvider.OpcServer ); });
	providers = resource<EProvider[], {}>( {loader: async () =>await this.authService.providers( console.log )} );

	fb = inject(FormBuilder);
	private get googleCredential():string{return this.envService.get("googleCredential"); }
	private showedGoogleLogin = false;
  form = this.fb.group({ username: [''], password: [''] });
	router = inject(Router);
	user = computed<User | null>( () => {
		return this.authService.user();
	});
}