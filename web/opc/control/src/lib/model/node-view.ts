import { Field, FieldKind, Operator, TableSchema, View, ViewType } from 'jde-framework';
import { ENodeClass, UaNode, Variable } from './node';
import { NodeId } from './node-id';
import { EAccess, ETypes } from './types';
import { valueString } from './value';

//A browsed node list has no GraphQL collection behind it, so - as LogView does for the log stream - the view runs over a
//hand-built schema and applies its filters and sort in the browser rather than through query().
export class NodeView extends View{
	static readonly collectionName = "opcNodes";//profile keys `opcNodes/views` and `opcNodes/viewIndex`:  one set of views for every node on every connection, the way a ql-list collection has one
	static columns:Record<string,string> = { id: "ID", name: "Name", class: "Class", dataType: "Data Type", snapshot: "Snapshot", access: "Access", description: "Description" };
	//every column a non-null String:  ids and values are of mixed types and are filtered on their cell text, and NON_NULL keeps the <null>/<not null> suggestions out of the filter tab
	static schema:TableSchema = new TableSchema( { name: "OpcNode", fields: Object.keys( NodeView.columns ).map( name=>new Field({name, ofType: {kind: FieldKind.SCALAR, name: "String"}}) ) } );
	static default():NodeView{
		const view = new NodeView( {configColumns: ["id", "name", {name: "class", hidden: true}, {name: "dataType", hidden: true}, "snapshot", {name: "access", hidden: true}, "description"], sort: [{active: "name", direction: "asc"}]}, NodeView.schema );
		view.showSelector = true;//the subscription checkboxes
		view.type = ViewType.System;
		return view;
	}

	get displayedColumns():string[]{ return (this.showSelector ? ["select"] : []).concat( this.fields.filter(f=>f.displayed).map(f=>f.name) ); }

	//the rows in view order:  filtered, then sorted by each Sort in turn (the first is primary)
	apply( nodes:readonly UaNode[] ):UaNode[]{
		const rows = nodes.filter( n=>this.matches(n) );
		const sorts = this.sort.filter( s=>s.active && s.direction );
		if( sorts.length ){
			rows.sort( (a,b)=>{
				for( const s of sorts ){
					const c = NodeView.compare( NodeView.cellValue(a, s.active), NodeView.cellValue(b, s.active) );
					if( c )
						return s.direction=="asc" ? c : -c;
				}
				return 0;
			});
		}
		return rows;
	}
	//In/NotIn match the cell text exactly, as the SQL `in` a ql-list filter turns into;  the comparisons go numeric when both sides are numbers
	matches( node:UaNode ):boolean{
		return this.fieldFilters.every( ff=>{
			const values = ff.filter.value;
			if( !values.length )
				return true;
			const cell = NodeView.cellValue( node, ff.field.name );
			const text = cell==null ? "" : String( cell );
			switch( ff.filter.operator ){
				case Operator.NotIn:           return !values.some( v=>String(v)==text );
				case Operator.Less:            return NodeView.compare( cell, values[0] )<0;
				case Operator.LessOrEqual:     return NodeView.compare( cell, values[0] )<=0;
				case Operator.Greater:         return NodeView.compare( cell, values[0] )>0;
				case Operator.GreaterOrEqual:  return NodeView.compare( cell, values[0] )>=0;
				default:                       return values.some( v=>String(v)==text );//In - and None, which the filter tab never emits
			}
		});
	}
	//what a column shows for a node - the filter and sort work on this, so they agree with the screen.  A numeric value stays a number so it sorts and compares as one.
	static cellValue( node:UaNode, column:string ):string|number|undefined{
		const variable = node.isVariable ? node as Variable : undefined;
		switch( column ){
			case "id":          return typeof node.id=="number" ? node.id : node.id?.toString();
			case "name":        return node.name;
			case "class":       return ENodeClass[node.nodeClass];
			case "dataType":    return NodeView.dataType( variable );
			case "snapshot":    return typeof variable?.value=="number" ? variable.value : variable?.value===undefined ? undefined : valueString( variable?.value );
			case "access":      return NodeView.access( variable );
			case "description": return node.description?.text;
		}
		return undefined;
	}
	static dataType( variable:Variable|undefined ):string|undefined{
		if( !variable )
			return undefined;
		const custom = variable.customDataType;
		return custom ? (custom instanceof NodeId ? custom : custom.id).uaString() : ETypes[variable.dataType!] ?? `${variable.dataType}`;
	}
	static access( variable:Variable|undefined ):string|undefined{
		if( !variable )
			return undefined;
		const level = variable.userAccessLevel ?? EAccess.None;
		return NodeView.accessNames.filter( ([flag])=>level & flag ).map( ([,name])=>name ).join( ", " );
	}
	private static accessNames:[EAccess,string][] = [[EAccess.Read, "Read"], [EAccess.Write, "Write"], [EAccess.HistoryRead, "History Read"], [EAccess.HistoryWrite, "History Write"]];
	private static compare( a:unknown, b:unknown ):number{
		if( a==null || b==null )
			return a==null ? (b==null ? 0 : -1) : 1;//blanks sort first, as a null does
		const na = NodeView.toNumber( a ), nb = NodeView.toNumber( b );
		return !isNaN(na) && !isNaN(nb) ? na-nb : String( a ).localeCompare( String(b), undefined, {numeric: true, sensitivity: "base"} );
	}
	private static toNumber( x:unknown ):number{ return typeof x=="number" ? x : typeof x=="string" && x.trim()!=="" ? Number( x ) : NaN; }//Number("") is 0, which would rank an empty cell among the numbers
}
