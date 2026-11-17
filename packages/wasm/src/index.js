const DEFAULT_BASE_URL = '/wasm/';

/**
 * Load the Emscripten module produced by the package build.
 * The generated files are copied to the docs site's public/wasm directory
 * during the docs preparation step.
 */
export async function createAbelRuntime({ baseUrl = DEFAULT_BASE_URL } = {}) {
  const normalizedBase = baseUrl.endsWith('/') ? baseUrl : `${baseUrl}/`;
  const moduleUrl = `${normalizedBase}abel.js`;
  const wasmUrl = `${normalizedBase}abel.wasm`;
  const response = await fetch(moduleUrl);
  if (!response.ok) {
    throw new Error(`could not fetch ${moduleUrl} (${response.status})`);
  }

  // Keep this import outside Vite's static dependency graph. The generated
  // ESM file lives in `public/wasm` and must be fetched by the browser as-is.
  const loadModule = new Function('url', 'return import(url);');
  const module = await loadModule(moduleUrl);
  const factory = module.default ?? module;
  return factory({
    locateFile: (file) => file.endsWith('.wasm') ? wasmUrl : `${normalizedBase}${file}`,
  });
}

export async function run(source, options) {
  const runtime = await createAbelRuntime(options);
  return runtime.run(source);
}

export async function runNotebook(sources, options) {
  const runtime = await createAbelRuntime(options);
  const input = new runtime.StringVector();
  sources.forEach((source) => input.push_back(source));
  try {
    const cells = runtime.runNotebook(input);
    return Array.from(cells, (cell) => ({
      ok: cell.ok,
      output: cell.output,
      value: cell.value,
      error: cell.error,
    }));
  } finally {
    input.delete();
  }
}
