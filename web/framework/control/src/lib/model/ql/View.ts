import { verify, Query, StringUtils, TableSettings } from "jde-framework";
import { Field, FieldKind } from "./schema/Field";
import { TableSchema } from "./schema/TableSchema";
import { Sort } from "@angular/material/sort";

export class Days{
	constructor( startDate:Date ){
		let eod = new Date();
		eod.setHours( 0, 0, 0, 0 );
		eod.setDate( eod.getDate()+1 );
		let bod = new Date( startDate );
		bod.setHours( 0, 0, 0, 0 );
		this.days = Math.round( (eod.getTime()-bod.getTime())/(1000*60*60*24) );
	}
	fromNow():Date{
		let start = new Date();
		start.setHours( 0, 0, 0, 0 );
		start.setDate( start.getDate()-this.days+1 );
		return start;
	}
	days:number;
}
export type DbScalar = string|number|boolean|null|Date|Days;
export enum Operator{
	None,
	In,
	NotIn,
	Less,
	LessOrEqual,
	Greater,
	GreaterOrEqual,
	Regex,
	ElementMatch,
	Between
}
//hidden = queried but not necessarily shown eg id
export type ViewFieldSettings = {name?:string, displayName?:string, style?:Style, defaultView?:boolean/*=!hidden*/, hidden?:boolean};
type ViewFieldJson = {name:string, hidden?:boolean, displayName?:string, style?:Style};
export type Filter = { operator: Operator, value: DbScalar[] };
export class Flex{
	constructor( value:string|number ){
		if( typeof value === "string" ){
			if( value=="1" ){
				this.flexGrow = this.flexShrink = 1;
				this.flexBasis = "0%";
			}else if( value=="initial" ){
				this.flexGrow = 0;
				this.flexShrink = 1;
				this.flexBasis = "auto";
			}
			else if( value=="none" ){
				this.flexGrow = this.flexShrink = 0;
				this.flexBasis = "auto";
			}
			else {
				let parts = value.split(" ");
				verify( parts.length==3, `Invalid flex string: ${value}` );
				this.flexGrow = parseFloat( parts[0] );
				this.flexShrink = parseFloat( parts[1] );
				this.flexBasis = parts[2];
			}
		}else{
			this.flexBasis = `${value}px`;
		}
	}
	toString(){
		return `${this.flexGrow ?? 0} ${this.flexShrink ?? 0} ${this.flexBasis ?? "auto"}`;
	}
	flexGrow?:number;
	flexShrink?:number;
	flexBasis?:string
};
export class Style{
	constructor( value:Partial<Style>|number ){
		if( typeof value === "number" )
			this.flex = new Flex( value );
		else
			Object.assign( this, value );
	}
	toJSON(){
		return this.flex ? {flex: this.flex.toString()} : {};
	}
	flex?: Flex;
}
export class ViewField{
	constructor( args: {qlField: Field, settings?: ViewFieldSettings}|{field: ViewFieldJson, schema: TableSchema}|ViewField ){
		if( args instanceof ViewField ){
			const copyFrom = args as ViewField;
			this.qlField = copyFrom.qlField;
			this.style = copyFrom.style;
			this._displayed = copyFrom._displayed;
			this.displayName = copyFrom.displayName;
		}else if( "qlField" in args ){
			const settings = args.settings;
			this.qlField = args.qlField;
			this.style = settings?.style;
			this._displayed = settings?.hidden ? false : undefined;
			this.displayName = settings?.displayName ?? StringUtils.idToDisplay( this.name );
		}else{
			const serialized = args as {field: ViewFieldJson, schema: TableSchema};
			const json = serialized.field;
			this.qlField = serialized.schema.fields.find( f=>f.name==json.name )!;
			this.style = json.style;
			this._displayed = json.hidden ? false : undefined;
			this.displayName = json.displayName ?? StringUtils.idToDisplay( this.name );
		}
	}
	toJson( customDisplay:boolean=false ):ViewFieldJson{
		let y: ViewFieldJson = { name: this.name };
		if( !this.displayed )
			y["hidden"] = true;
		if( customDisplay )
			y["displayName"] = this.displayName;
		if( this.style )
			y["style"] = this.style;
		return y;
	}
	get displayed():boolean{ return this._displayed ?? (this.type.ofType?.name!="ID" && this.name!="attributes" && this.type.kind!=FieldKind.LIST); }
	set displayed(x){this._displayed=x;} private _displayed:boolean|undefined;

	qlField: Field;
	get name(){ return this.qlField.name; }
	displayName:string;
	get type(){ return this.qlField.type; }
	style?: Style;
};

type ViewConfigArgs = { configColumns:(string|ViewFieldSettings)[], sort:Sort[], filters?:{name: string, filter: Filter} };
type ViewJson = { name:string|undefined, collectionName:string, fields:ViewFieldJson[], filters:{ field?: Field, name:string, filter: Filter }[], limit?:number, showSelector:boolean|undefined, sort:Sort[]|undefined };
export type ViewSerializedArgs = { name:string|undefined, collectionName:string, fields:ViewField[], limit?:number, showSelector:boolean, sort:Sort[], filters?:{name: string, filter: Filter}[] };
export type FieldFilter = { field: Field, filter: Filter };
export enum ViewType{
	System,
	Adhoc,
	User
}
export class View{
	constructor( value:ViewConfigArgs|ViewSerializedArgs|View, schema?:TableSchema ){
		if( value instanceof View )
			this.copyConstructor( value as View );
		else if( (value as ViewConfigArgs).configColumns )
			this.configConstructor( value as ViewConfigArgs, schema! );
		else if( (value as ViewSerializedArgs).fields )
			this.serializedConstructor( value as ViewSerializedArgs, schema! );
	}
	private copyConstructor( view:View ):void{
		this.name = view.name;
		this.collectionName = view.collectionName;
		this.limit = view.limit;
		this.fields = [...view.fields];
		if( view.fieldFilters )
			this.fieldFilters = [...view.fieldFilters];
		this.showSelector = view.showSelector;
		this.sort = structuredClone( view.sort );
		this.type = view.type;
	}
	private configConstructor( config:ViewConfigArgs, schema:TableSchema ):void{
		this.sort = config.sort;
		this.collectionName = schema.collectionName;
		this.fields = this.columns( schema, config.configColumns, ["id", "attributes"] );
	}
	private serializedConstructor( config:ViewSerializedArgs, schema:TableSchema ):void{
		this.name = config.name;
		this.collectionName = config.collectionName;
		if( config.limit )
			this.limit = config.limit;
		this.fields = config.fields.map( f=>new ViewField({field: f, schema: schema}) );
		for( let fieldFilter of config.filters ?? [] )//{name: string, filter: Filter}
			this.fieldFilters.push( {field: schema.fields.find(f=>f.name==fieldFilter.name)!, filter: fieldFilter.filter} );

		this.showSelector = config.showSelector;
		this.sort = config.sort;
		this.type = ViewType.User;
	}
	private columnsToQuery( excludedColumns:string[], includeDeleted:boolean ):ViewField[]{
		return this.fields.filter( (x)=>
			(x.displayed || x.name=="id")
		&& !excludedColumns?.includes(x.name)
		&& (includeDeleted || x.name!="deleted") );
	}
	private columnsToQuerySorted( orderedSpec:string[], excludedColumns:string[]=[], includeDeleted:boolean=false ):ViewField[]{
		const notInSpec = this.fields.filter( f=>!orderedSpec.includes(f.name) ).map( f=>f.name );
		let sort = [...orderedSpec, ...notInSpec ];
		const f = ( x:ViewField,y:ViewField )=>{ return sort.indexOf( x.name )-sort.indexOf( y.name ); };
		return this.columnsToQuery( excludedColumns, includeDeleted ).sort( f );
	}
	private columns( schema:TableSchema, configColumns:(string|ViewFieldSettings)[], defaultHidden:string[] ):ViewField[]{
		let selectCols = [];
		let description; //want last
		let id; //want first
		for( let col of configColumns ){
			const fieldName = typeof col=="string" ? col : col.name;
			const settings = typeof col=="string" ? {} : col;
			const field = schema.fields.find( f=>f.name==fieldName )!;
			const viewField = new ViewField( {qlField: field, settings: settings} );
			if( field.name=="description" )
				description = viewField;
			else
				selectCols.push( new ViewField({qlField:field, settings: settings}) );
		}
		for( let field of schema.fields.filter( f=>!selectCols.find( (c)=>c.name==f.name ) && (f.name=="id" || f.name=="deleted") ) ){
			const viewField = new ViewField( {qlField:field, settings: {name: field.name, hidden: true}} );
			if( field.name=="id" )
				id = viewField;
			else
				selectCols.push( viewField );
		}
		if( id )
			selectCols.unshift( id );
		if( description )
			selectCols.push( description );
		return selectCols;
	}
	private parseQuery( query:string, vars:any, schema:TableSchema ){
		const addField = ( fieldName:string, hidden=true )=>{
			let view = this.fields.find( f=>f.name==fieldName );
			if( view )
				return view;
			let field = schema.fields.find( f=>f.name==fieldName );
			if( field ){
				view = new ViewField( {qlField:field, settings: {name: field.name}} );
				if( !hidden )
					view.displayed = true;
				this.fields.push( view );
			}
			return view;
		}
		let fieldNames = query.substring( query.indexOf("{")+1, query.lastIndexOf("}") ).split(" ").filter( x=>x.trim().length>0 );
		for( let fieldName of fieldNames )
			addField( fieldName, fieldName=="id" );
		let argsStart = query.indexOf("(");
		let args = argsStart>0
			? query.substring( argsStart+1, query.indexOf(")") ).split(",").map( x=>x.trim() )
			: [];
		for( let arg of args ){
			let parts = arg.split(":").map( x=>x.trim() );
			if( parts.length!=2 )
				continue;
			let name = parts[0];
			let value = vars[ parts[1].substring(1) ];
			if( name=="limit" )
				this.limit = +value;
			else if( name=="orderBy" ){
				this.sort = value;
				for( let item of JSON.parse(value) as Record<string,"ASC"|"DESC">[] )
					addField( Object.keys(item)[0] );
			}
		}
		// this.fields = fieldNames.map( name => {
		// 	let field = schema.fields.find( f=>f.name==name );
		// 	if( !field )
		// 		return ;
		// 	return { field: field } as ViewField;
		// } );
	}
	query( showDeleted:boolean, skip:number ):Query{
		let deletedField = this.fields.find( f=>f.name=="deleted" );
		if( deletedField )
				deletedField.displayed = showDeleted;

		let fieldStr = this.fields.filter( f=>f.displayed || f.name=="id" ).map( f=>f.name ).join(" ");
		let args = [];
		let vars:Record<string, DbScalar[]|null> = {};
		if( this.limit )
			args.push( `limit:${this.limit}` );
		if( skip )
			args.push( `skip:${skip}` );
		if( this.sort?.length ){
			let sortStr = '';
			for( let s of this.sort )
				sortStr += `{${s.active}:"${s.direction}"},`;
			args.push( `orderBy:[${sortStr.slice(0,-1)}]` );
		}

		for( const fieldFilter of this.fieldFilters ){
			const filter = fieldFilter.filter;
			const values = filter.value;
			if( !values.length )
				continue;
			const name = fieldFilter.field.name;
			if( values.length==1 && values[0]=="<not null>" )
				args.push( `${name}:{"ne":null}` );
			else{
				args.push( `${name}:$${name}` );
				verify( values.length>0 );
				vars[name] = [...values];
			}
		}
		const iDeletedArg = args.findIndex( a=>a.startsWith("deleted:") );
		if( showDeleted && iDeletedArg!=-1 ){
			args.splice( iDeletedArg, 1 );
			delete vars["deleted"];
		}
		else if( !showDeleted && iDeletedArg==-1 ){
			args.push( `deleted:$deleted` );
			vars["deleted"] = null;
		}

		return { text: `${this.collectionName}${args.length>0 ? `(${args.join(",")})` : ""}{ ${fieldStr} }`, vars: vars };
	}
	append( fields: Field[] ){
		for( let field of fields.filter(f=>!this.fields.find(v=>v.name==f.name)) )
			this.fields.push( new ViewField({qlField: field, settings: {name: field.name, hidden: true}}) );
	}
	setDeletedDisplayed( show:boolean ){ this.fields.find(f=>f.name=="deleted")!.displayed = show; }
	toJson( defaultSettings:TableSettings|undefined ):ViewJson{
		let fields = [];
		for( let field of this.fields ){
			const settings = defaultSettings?.columns!.find( c=>typeof c=="string" ? null : c.name==field.name ) as ViewFieldSettings;
			const customDisplay = (settings?.displayName ?? StringUtils.idToDisplay( field.name ))!=field.displayName;
			if( field.displayed || customDisplay )
				fields.push( field.toJson(customDisplay) );
		}
		let filters = new Array<{field?: Field, name:string,filter:Filter}>;
		for( let ff of this.fieldFilters )
			filters.push( {name: ff.field.name, filter: ff.filter} );
		let sort = defaultSettings && JSON.stringify(this.sort)==JSON.stringify(defaultSettings.sort) ? undefined : this.sort;
		return {
			name: this.name,
			filters: filters,
			collectionName: this.collectionName,
			fields: fields,
			limit: this.limit,
			showSelector: this.showSelector,
			sort: sort
		};
	}
	fieldFilters:FieldFilter[] = [];
	name:string|undefined;
	collectionName!:string;
	fields:ViewField[]=[];
	limit:number=25;
	get isUser(){ return this.type==ViewType.User; }
	get isSystem(){ return this.type==ViewType.System; }
	get isAdhoc(){ return this.type==ViewType.Adhoc; }
	showSelector:boolean=false;
	sort:Sort[] = [];
	type=ViewType.System;
};
