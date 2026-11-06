# Auto Decorator

A Geode mod for Geometry Dash 2.2081 that adds a **Deco** button to the level editor.

- **Decorate**: places decorations on top of every block with open space above, and (optionally) upside-down decorations under floating blocks.
- **Remove**: deletes every decoration the mod added (they're all tagged with group 9999 by default).

Change density, object IDs and the group in Geode → Auto Decorator → Settings.

## Building

Push this folder to a GitHub repo. The workflow in `.github/workflows/build.yml` builds it for Windows, Mac and Android and uploads `mynor.auto-decorator.geode` as an artifact under the **Actions** tab.
