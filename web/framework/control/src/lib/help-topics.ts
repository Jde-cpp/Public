import { HelpTopic } from 'jde-spa';

//jde-framework's pages, registered by the site under HELP_TOPICS.  The markdown ships from assets/help (web/CLAUDE.md).
export const frameworkHelpTopics:HelpTopic[] = [
	{ id: 'apps', title: 'Applications', summary: 'Running services, their logs and log levels', icon: 'apps', url: 'assets/jde-framework/help/apps.md', routes: ['apps', 'apps/appServers/:instance'] }
];
