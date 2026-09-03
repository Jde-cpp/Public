import { KeyValue } from '@angular/common';
import { Pipe, PipeTransform } from '@angular/core';

@Pipe({name: 'enumKeys'})
export class EnumKeysPipe implements PipeTransform {
	//a numeric enum object, whose reverse mapping is what this reads:  the numeric keys hold the NAMES.
	transform(value: Record<string,string|number>): KeyValue<number,string>[] {
		let y = new Array<KeyValue<number,string>>();
		for( const key in value ){
			if( !isNaN(Number(key)) )
				y.push( {key:Number(key), value:String(value[key])} );
		}
		return y;
	}
}