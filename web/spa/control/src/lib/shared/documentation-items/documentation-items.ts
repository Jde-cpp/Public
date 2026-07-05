import {Injectable} from '@angular/core';
/*import {EXAMPLE_COMPONENTS} from '@angular/components-examples';

export interface AdditionalApiDoc {
  name: string;
  path: string;
}

export interface ExampleSpecs {
  prefix: string;
  exclude?: string[];
}*/

export interface RouteItem {
  /** Id of the doc item. Used in the URL for linking to the doc. */
  id: string;
  /** Display name of the doc item. */
  name: string;
  /** Short summary of the doc item. */
  summary?: string;
	externalRedirect?: string;
  /** Package which contains the doc item.
  packageName?: string;
  /** Specifications for which examples to be load.
  //exampleSpecs: ExampleSpecs;
   List of examples.
  examples?: string[];
	*/

}

export interface DocSection {
  name: string;
  summary: string;
}
const SettingsKey = 'settings';

const DOCS: { [key: string]: RouteItem[] } = {
  [SettingsKey]: [
		{id: 'applications',name: 'Applications',summary: 'View Applications.'},
		{id: 'logs',name: 'Logs',summary: 'View logs.'}
  ],
};

// for( let doc of DOCS[SettingsKey] ){
//   doc.packageName = SettingsKey;
// }

	const AllSettings = DOCS[SettingsKey];
	const ALL_DOCS = AllSettings;

@Injectable({providedIn: 'root'})
export class DocumentationItems {

  getItems(section: string): RouteItem[] {
    if (section === SettingsKey) {
      return AllSettings;
    }
/*    if (section === CDK) {
      return ALL_CDK;
    }*/
    return [];
  }

  getItemById(id: string, section: string): RouteItem | undefined {
		let item;
		if( !item )
			console.error( "item==null" );
		return item;
  }
}
/*
function processDocs(packageName: string, docs: RouteItem[]): RouteItem[] {
  for (const doc of docs) {
    doc.packageName = packageName;
    doc.hasStyling ??= packageName === 'material';
    doc.examples = exampleNames.filter(key =>
      key.match(RegExp(`^${doc.exampleSpecs.prefix}`)) &&
      !doc.exampleSpecs.exclude?.some(excludeName => key.indexOf(excludeName) === 0));
  }

  return docs.sort((a, b) => a.name.localeCompare(b.name, 'en'));
}*/
