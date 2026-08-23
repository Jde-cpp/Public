import { Signal } from '@angular/core';

//Server-side persistence for ProfileStore, provided under the 'IProfileService' token (implemented in jde-framework over AppService).
export interface IProfileService{
	userKey: Signal<string|undefined>; //stable id of the logged-in user; undefined ⇒ not logged in ⇒ ProfileStore stays on localStorage
	load( key:string ):Promise<string|null>; //raw JSON string; null ⇒ no server row
	save( key:string, value:string|null ):Promise<void>; //null deletes the row
}
