# Access

The [Access](/access) section is where users are given rights. Rights are granted through **permissions** - an allow or deny of an action on a **resource** - and permissions are collected into **roles**, which are given to **groups** and to **users**. Every right a user holds comes through a role.

## Lists

Users, groups, roles and resources each have a list page. **Add** creates a new record, the refresh button reloads the list, and the view buttons choose which columns are shown and how they are sorted; a saved view is kept with your profile.

Click a row to open the record.

## Users

A user's page has four tabs: **Properties**, the **Groups** the user belongs to, the **Roles** granted to the user directly, and **Effective rights** - what the user can actually do on each resource through every group and role, the same answer the server enforces. Hover a mark to see the grants behind it: a role, a role via a group, or a direct grant. An enforced resource the user has no grant on is listed as no access. Rights granted on individual OPC nodes are folded under the node table's row; expand it to see them, each named and linking to its page.

## Groups

A group's page has **Properties**, its **Users**, the child **Groups** nested in it and the **Roles** granted to it.

## Roles

A role's page has its properties, the **Permissions** it carries, the child **Roles** it includes, and the **Groups** and **Users** it is granted to. Removing a role also removes the roles nested under it.

## Resources

A resource is something a permission can name: a page of this site, a record type, or an OPC node. Nodes are secured from their own page, see [Gateways](/help/gateways). The list is one row per record type with its **Enforced** switch; node-scoped resources are not listed here - they are managed from the node's page and shown on a user's Effective rights tab.

Changes on a record page are saved with the **Save** button; **Cancel** discards them and returns to the list.
