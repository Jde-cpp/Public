import { HelpTopic } from 'jde-spa';

//jde-access's pages, registered by the site under HELP_TOPICS.  The markdown ships from assets/help (web/CLAUDE.md).
export const accessHelpTopics:HelpTopic[] = [
	{ id: 'access', title: 'Access', summary: 'Users, groups, roles and resources', icon: 'admin_panel_settings', url: 'assets/jde-access/help/access.md', routes: ['access'] }
];
