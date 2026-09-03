import {Component, OnInit, input, output} from '@angular/core';
import {MatChipListbox, MatChipsModule} from '@angular/material/chips';
import {MatOption, MatSelect, MatSelectModule} from '@angular/material/select';
import { CircularBuffer } from '../../utils/collections'
import { KeyValuePipe } from '@angular/common';

@Component( {
	selector: 'link-select',
	templateUrl: 'link-select.html',
	imports:[KeyValuePipe,MatChipListbox, MatChipsModule,MatOption,MatSelect] })
export class LinkSelect<TOptionKey> implements OnInit{
	ngOnInit()
	{}
	valueChange( selectedId:TOptionKey ){
		this.selected = selectedId;
		this.selectChange.emit( selectedId );
		this.links.unshift( selectedId );
	}

	placeholder = input<string>( "Date range" );
	get links(){ return this.options().links; }
	get selected(){ return this.options().selected; } set selected(x){ if( this.options().selected!=x ) this.options().selected=x; }
	selectChange = output<TOptionKey>();
	options = input.required<LinkSelectOptions<TOptionKey>>();
	get linkValues():Map<TOptionKey,string>
	{
		let y=new Map<TOptionKey,string>();
		this.links?.forEach( x =>
		{
			if( x!=this.selected && this.options().values.has(x) )
				y.set( x, this.options().values.get(x)! );
		});
		return y;
	}
}
export class LinkSelectOptions<TOptionKey>
{
	constructor( private readonly _values:Map<TOptionKey,string>, linkCount=3, public isValid?:(key:TOptionKey,value:string)=>boolean )
	{
		this.links = new CircularBuffer<TOptionKey>( linkCount );
		this.fillLinks();
		this.selected = _values.keys().next().value!;
		//this.links.push( this.selected );
	}
	assign( other:LinkSelectOptions<TOptionKey> )
	{
		this.selected = other.selected;
		this.links.assign( other.links );
		this.fillLinks();
	}
	fillLinks()
	{
		for( let [key,value] of this.values )
		{
			if( this.links.length>=this.links.maxLength )
			    break;
			//const key = ret.value;
			if( this.links.indexOf(key)==-1 && this.selected!=key && (!this.isValid || this.isValid(key,value)) )
				this.links.push( key );
		}
	}
	get values():Map<TOptionKey,string>
	{
		if( !this.isValid )
			return this._values;
		var values=new Map<TOptionKey,string>();
		this._values.forEach( (value,key)=>{ if(this.isValid!(key,value)) values.set(key,value);} );
		return values;
	}
	selected:TOptionKey;
	links:CircularBuffer<TOptionKey>;
}