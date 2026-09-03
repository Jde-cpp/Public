import {Component,CUSTOM_ELEMENTS_SCHEMA,input,model} from '@angular/core';
//import { BrowserModule } from '@angular/platform-browser'
import { MatFormFieldModule } from '@angular/material/form-field';
import {MatSelectModule} from '@angular/material/select';
import * as FromServer from 'jde-proto/App.FromServer';
import { ELogLevel } from 'jde-proto/Log';
import { MatChip, MatChipListbox } from '@angular/material/chips';

@Component({
    selector: 'severity-picker',
    templateUrl: './severity-picker.html',
    imports: [MatChip,MatChipListbox,MatFormFieldModule, MatSelectModule],
		schemas: [ CUSTOM_ELEMENTS_SCHEMA ]
})
export class SeverityPicker{
	onSelectionChange( value:ELogLevel ){
		this.level.set( value );//model() emits levelChange on set, and only on a set from HERE
	}
	//model(), so `levelChange` fires for the user's pick and NOT for the parent writing [level].  The old @Input setter
	//emitted on every write after the first, so a parent-driven update bounced straight back at it - and the chip path
	//emitted twice, once from the setter and once from onSelectionChange (review3 C7).
	level = model.required<ELogLevel>();
	isSelect = input<boolean>( true );

	options:LogOption[]=[{name:'Trace',value:ELogLevel.Trace},{name:'Debug',value:ELogLevel.Debug}, {name:'Info',value:ELogLevel.Information},{name:'Warning',value:ELogLevel.Warning},{name:'Error',value:ELogLevel.Error},{name:'Critical',value:ELogLevel.Critical},{name:'None',value:ELogLevel.NoLog}];
}

interface LogOption
{
	name:string;
	value:ELogLevel;
}
