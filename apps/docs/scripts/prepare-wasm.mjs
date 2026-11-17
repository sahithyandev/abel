import { cp, mkdir, readdir } from "node:fs/promises";
import { join, resolve } from "node:path";

const packageDist = resolve("../../packages/wasm/dist");
const publicDist = resolve("public/wasm");

try {
  const files = await readdir(packageDist);
  const runtimeFiles = files.filter(
    (file) => file === "abel.js" || file === "abel.wasm",
  );
  if (runtimeFiles.length !== 2) {
    throw new Error("expected both abel.js and abel.wasm");
  }
  await mkdir(publicDist, { recursive: true });
  await Promise.all(
    runtimeFiles.map((file) =>
      cp(join(packageDist, file), join(publicDist, file)),
    ),
  );
} catch (error) {
  throw new Error(
    `WASM runtime is not built. Run ` +
      "`bun --cwd packages/wasm run build` before building the docs site.\n" +
      error.message,
  );
}
