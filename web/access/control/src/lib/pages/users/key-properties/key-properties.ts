import { Component, computed, inject, input, model, signal } from '@angular/core';
import { DatePipe } from '@angular/common';
import { MatButtonModule } from '@angular/material/button';
import { MatChipsModule } from '@angular/material/chips';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatIcon } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatSelectModule } from '@angular/material/select';

import { SnackbarService, TableSchema } from 'jde-framework';

import { User } from '../../../model/user';

type ExpiryStatus = 'valid' | 'expiring' | 'expired';

@Component( {
	selector: 'key-properties',
	templateUrl: './key-properties.html',
	styleUrls: ['./key-properties.scss'],
	imports: [DatePipe, MatButtonModule, MatChipsModule, MatFormFieldModule, MatIcon, MatInputModule, MatSelectModule]
})
export class KeyProperties{
	update( field:keyof User, value:unknown ){
		this.record.set( new User({...this.record(), [field]: value}).properties );
	}

	async copy( value:string, what:string ){
		try{
			await navigator.clipboard.writeText( value );
			this.snackbar.info( `${what} copied` );
		}catch( e ){
			this.snackbar.exception( `Copy ${what} failed.`, e );
		}
	}

	record = model.required<Partial<User>>();
	schema = input.required<TableSchema>();
	showModulus = signal( false );

	providers = computed( ()=>this.schema().enums.get("Provider") ?? [] );
	providerId = computed<number>( ()=>{
		const value = this.record().provider as string|number|undefined;
		return typeof value=="number" ? value : this.providers().find( (x)=>x.name==value )?.id ?? 0;
	});

	modulus = computed( ()=>String(this.record().modulus ?? "") );
	keyBits = computed( ()=>this.modulus().length*4 );
	modulusDisplay = computed( ()=>this.showModulus() ? this.modulus() : `${this.modulus().slice(0, 32)}…` );

	hasCert = computed( ()=>!!(this.record().distinguished || this.record().issuer || this.record().subjectAlt || this.record().expiration) );
	selfSigned = computed( ()=>!!this.record().issuer && this.record().issuer==this.record().distinguished );
	subjectAltNames = computed( ()=>(this.record().subjectAlt ?? "").split(",").map( (x)=>x.trim() ).filter( (x)=>x.length ) );
	expiry = computed( ()=>{
		const raw = this.record().expiration;
		if( !raw )
			return undefined;
		const date = new Date( raw );
		const days = Math.floor( (date.getTime()-Date.now())/86400000 );
		const status:ExpiryStatus = days<0 ? 'expired' : days<30 ? 'expiring' : 'valid';
		const relative = days<0 ? `expired ${-days} day${-days==1 ? '' : 's'} ago` : days==0 ? 'expires today' : `expires in ${days} day${days==1 ? '' : 's'}`;
		return { date, status, relative };
	});

	private snackbar = inject( SnackbarService );
}
