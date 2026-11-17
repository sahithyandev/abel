import { defineConfig } from "astro/config";
import abelLanguage from "./src/abel-language.mjs";

const base = process.env.ASTRO_BASE ?? "/";

export default defineConfig({
  output: "static",
  base,
  markdown: {
    shikiConfig: {
      langs: [abelLanguage],
    },
  },
});
