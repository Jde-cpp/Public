
export class PageSettings{
	constructor( x:any ){
		this.excludedColumns = x.excludedColumns;
		this.name = x.name;
		this.showAdd = x.showAdd;
		this.canPurge = x.canPurge!==false;
		this.table = x.table ?? x.id;
	}
	canPurge:boolean;
	excludedColumns:string[];
	name:string;
	showAdd:boolean = true;
	table:string;
	get type():string{ return this.table ?? this.name[0].toLowerCase()+this.name.substring(1); }
}
