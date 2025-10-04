import {Component,CUSTOM_ELEMENTS_SCHEMA,EventEmitter,Input,NgModule,Output} from '@angular/core';
//import { BrowserModule } from '@angular/platform-browser'
import { MatFormFieldModule } from '@angular/material/form-field';
import {MatSelectModule} from '@angular/material/select';
import * as AppFromServer from '../../proto/App.FromServer'; import FromServer = AppFromServer.Jde.App.Proto.FromServer;
import * as LogProto from '../../proto/Log'; import ELogLevel = LogProto.Jde.App.Log.Proto.ELogLevel;
import { NgFor } from '@angular/common';
import { MatChip, MatChipListbox } from '@angular/material/chips';

@Component({
    selector: 'severity-picker',
    templateUrl: './severity-picker.html',
    imports: [MatChip,MatChipListbox,MatFormFieldModule, MatSelectModule,NgFor],
		schemas: [ CUSTOM_ELEMENTS_SCHEMA ]
})
export class SeverityPicker{
	onSelectionChange( value:ELogLevel ){
		if( this.level!=value ){
			this.level=value;
			this.levelChange.emit( value );
		}
	}
	get level(){ return this.#level; } @Input() set level(x){ let emit = this.#level!==undefined; this.#level=x; if( emit )this.levelChange.emit( x ); }   #level:ELogLevel;
	@Output() levelChange = new EventEmitter<ELogLevel>();
	@Input() isSelect:boolean=true;

	options:LogOption[]=[{name:'Trace',value:ELogLevel.Trace},{name:'Debug',value:ELogLevel.Debug}, {name:'Info',value:ELogLevel.Information},{name:'Warning',value:ELogLevel.Warning},{name:'Error',value:ELogLevel.Error},{name:'Critical',value:ELogLevel.Critical},{name:'None',value:ELogLevel.NoLog}];
}

interface LogOption
{
	name:string;
	value:ELogLevel;
}
