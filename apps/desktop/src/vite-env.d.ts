/// <reference types="vite/client" />

declare module "@abel/wasm" {
  export function runNotebook(
    sources: string[],
    options?: { baseUrl?: string },
  ): Promise<
    Array<{
      ok: boolean;
      output: string;
      value: string;
      error: {
        category: string;
        message: string;
        line: number;
        column: number;
      };
    }>
  >;
}
