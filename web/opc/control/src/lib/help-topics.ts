import { HelpTopic } from 'jde-spa';

//jde-opc's pages, registered by the site under HELP_TOPICS.  The markdown ships from assets/help (web/CLAUDE.md).
//'gateways' covers the whole browse tree as a prefix; the two apps/ patterns outrank jde-framework's 'apps' by literal count.
export const opcHelpTopics:HelpTopic[] = [
	{ id: 'gateways', title: 'Gateways', summary: 'Gateways, server connections and node browsing', icon: 'hub', url: 'assets/jde-opc/help/gateways.md', routes: ['gateways', 'apps/gateways/:instance', 'apps/opcServers/:instance'] }
];
