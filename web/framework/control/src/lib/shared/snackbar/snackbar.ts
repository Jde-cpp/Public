import { Component, OnDestroy, ViewEncapsulation, inject, signal } from '@angular/core';
import { MatIconButton } from '@angular/material/button';
import { MatIcon } from '@angular/material/icon';
import { MAT_SNACK_BAR_DATA, MatSnackBarAction, MatSnackBarActions, MatSnackBarLabel, MatSnackBarRef } from '@angular/material/snack-bar';

/** duration<=0 (or omitted) => never auto-dismiss */
export type SnackbarData = { message:string, duration?:number };

@Component({
	template: `
		<span matSnackBarLabel>{{data.message}}</span>
		<span matSnackBarActions>
			<button matIconButton matSnackBarAction (click)='togglePin()' [attr.aria-pressed]='pinned()' [class.pinned]='pinned()' aria-label='Pin notification'>
				<mat-icon>push_pin</mat-icon>
			</button>
			<button matIconButton matSnackBarAction (click)='ref.dismiss()' aria-label='Close notification'>
				<mat-icon>close</mat-icon>
			</button>
		</span>`,
	styleUrl: './snackbar.scss',
	encapsulation: ViewEncapsulation.None,
	host: {class: 'jde-snackbar'},
	imports: [MatIconButton, MatIcon, MatSnackBarLabel, MatSnackBarActions, MatSnackBarAction]
})
export class Snackbar implements OnDestroy {
	ref = inject<MatSnackBarRef<Snackbar>>( MatSnackBarRef );
	data = inject<SnackbarData>( MAT_SNACK_BAR_DATA );

	constructor(){
		this.startTimer();
	}
	ngOnDestroy(){
		this.clearTimer();
	}
	togglePin(){
		this.pinned.update( p=>!p );
		this.pinned() ? this.clearTimer() : this.startTimer();
	}
	private startTimer(){
		if( this.data.duration && this.data.duration>0 )
			this.timeoutId = setTimeout( ()=>this.ref.dismiss(), this.data.duration );
	}
	private clearTimer(){
		if( this.timeoutId!=undefined ){
			clearTimeout( this.timeoutId );
			this.timeoutId = undefined;
		}
	}
	pinned = signal( false );
	private timeoutId:ReturnType<typeof setTimeout>|undefined;
}
