# Abel Desktop

The Abel desktop app is a Tauri 2 + Preact notebook shell. It uses the same
WebAssembly runtime as the browser playground and does not change Abel's
language semantics.

## Development

Build the WebAssembly runtime first when its generated files are missing or
the runtime adapter has changed:

```sh
bun --cwd packages/wasm run build
```

Then start the desktop app:

```sh
bun --cwd apps/desktop run tauri dev
```

The frontend production build can be checked independently with:

```sh
bun --cwd apps/desktop run build
```

## Notebook controls

- `Run cell` or `⌘/Ctrl + Enter` evaluates through the selected cell.
- `Run all` evaluates every cell from the beginning.
- Editing a cell invalidates that cell and later results.
- `⌘/Ctrl + S` downloads the current document as `.abelnb` JSON.
- `Open` imports a `.abelnb` or JSON notebook.
