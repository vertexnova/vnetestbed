# Diagrams

Draw.io source files for the VneTestbed documentation.

## Export to PNG

The `testbed.md` document references PNG images. Export the `.drawio` files to PNG using one of these methods:

### Option 1: draw.io Desktop (macOS/Windows/Linux)

If [draw.io Desktop](https://github.com/jgraph/drawio-desktop/releases) is installed:

```bash
drawio -x -f png -o context.png context.drawio
drawio -x -f png -o api.png api.drawio
drawio -x -f png -o architecture.png architecture.drawio
```

Or export all at once:

```bash
drawio -x -f png -o . .
```

### Option 2: draw.io Web

1. Open [app.diagrams.net](https://app.diagrams.net)
2. File → Open from → Device → select each `.drawio` file
3. File → Export as → PNG
4. Save to this `diagrams/` folder

### Files

| Source | Output | Used in testbed.md | Contents |
|--------|--------|---------------------|----------|
| context.drawio | context.png | Overview | C4 system context: Application → VneTestbed (AppContext, LayerStack, IPlugin, ILayer, Registry) |
| api.drawio | api.png | API usage | Runner flow: Build AppContext → LayerStack + createAndPushLayers → loop (onUpdate, onRender, onGui*) |
| architecture.drawio | architecture.png | Project layout | Folder structure and build flow (examples: 01_hello_testbed, 02_plugin_runner) |
