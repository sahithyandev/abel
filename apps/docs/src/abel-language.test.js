import { describe, expect, test } from "bun:test";
import grammar from "./abel-language.mjs";

describe("Abel TextMate grammar", () => {
  test("declares the language identity and file extension", () => {
    expect(grammar.scopeName).toBe("source.abel");
    expect(grammar.fileTypes).toContain("abel");
  });

  test("covers the language's core token groups", () => {
    const groups = grammar.patterns.map((pattern) => pattern.include);
    expect(groups).toEqual([
      "#comments",
      "#strings",
      "#numbers",
      "#functions",
      "#keywords",
      "#constants",
      "#builtins",
      "#operators",
      "#punctuation",
    ]);
    expect(grammar.repository.keywords.patterns[0].match).toContain("while");
    expect(grammar.repository.builtins.patterns[0].match).toContain("sqrt");
  });
});
