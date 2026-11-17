import { describe, expect, test } from "bun:test";
import { createAbelRuntime } from "./index.js";

describe("WASM runtime loader", () => {
  test("normalizes the base URL and locates the wasm binary", async () => {
    const requested = [];
    globalThis.fetch = async (url) => {
      requested.push(url);
      return { ok: true };
    };
    const runtime = await createAbelRuntime({
      baseUrl: "/assets/wasm",
      moduleLoader: async (url) => ({
        default: (options) => ({ options }),
      }),
    });
    expect(requested).toEqual(["/assets/wasm/abel.js"]);
    expect(runtime.options.locateFile("abel.wasm")).toBe(
      "/assets/wasm/abel.wasm",
    );
    expect(runtime.options.locateFile("helper.js")).toBe(
      "/assets/wasm/helper.js",
    );
  });

  test("reports a failed runtime fetch", async () => {
    globalThis.fetch = async () => ({ ok: false, status: 503 });
    await expect(createAbelRuntime({ baseUrl: "/wasm/" })).rejects.toThrow(
      "could not fetch /wasm/abel.js (503)",
    );
  });
});
