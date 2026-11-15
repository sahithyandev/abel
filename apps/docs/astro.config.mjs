import { defineConfig } from 'astro/config';
import abelLanguage from './src/abel-language.mjs';

export default defineConfig({
  output: 'static',
  markdown: {
    shikiConfig: {
      langs: [abelLanguage],
    },
  },
});
