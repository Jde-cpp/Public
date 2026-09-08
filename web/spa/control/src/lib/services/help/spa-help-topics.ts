import { HelpTopic } from './help-topic';

//jde-spa's own surface - the navbar and what every page shares.  routes [''] makes it the topic the ? button falls back to.
export const spaHelpTopics:HelpTopic[] = [
	{ id: 'navigation', title: 'Navigation', summary: 'Search, favorites, breadcrumbs and themes', icon: 'explore', url: 'assets/jde-spa/help/navigation.md', routes: [''] }
];
