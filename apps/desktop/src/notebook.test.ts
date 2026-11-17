import { describe, expect, test } from "bun:test";
import {
  makeCell,
  newNotebook,
  notebookPayload,
  parseNotebook,
} from "./notebook";

describe("notebook model", () => {
  test("creates the starter notebook", () => {
    const notebook = newNotebook();
    expect(notebook.format).toBe(1);
    expect(notebook.cells).toHaveLength(1);
    expect(notebook.cells[0].source).toContain("fn mean");
  });

  test("serializes only editable notebook data", () => {
    const notebook = newNotebook();
    notebook.cells[0].output = "should not be saved";
    notebook.cells[0].status = "success";
    expect(JSON.parse(notebookPayload(notebook))).toEqual({
      format: 1,
      cells: [{ id: notebook.cells[0].id, source: notebook.cells[0].source }],
    });
  });

  test("normalizes opened cells and rejects unsupported formats", () => {
    const opened = parseNotebook(
      JSON.stringify({ format: 1, cells: [{ source: "1 + 2" }] }),
      () => "generated",
    );
    expect(opened.cells[0]).toMatchObject({
      id: "generated",
      source: "1 + 2",
      status: "stale",
      output: "",
      value: "",
    });
    expect(() =>
      parseNotebook(JSON.stringify({ format: 2, cells: [] })),
    ).toThrow("unsupported notebook format");
    expect(() =>
      parseNotebook(JSON.stringify({ format: 1, cells: [{ source: 42 }] })),
    ).toThrow("invalid notebook cells");
    expect(() =>
      parseNotebook(JSON.stringify({ format: 1, cells: [null] })),
    ).toThrow("invalid notebook cells");
  });

  test("creates independent blank cells", () => {
    expect(makeCell().source).toBe("");
    expect(makeCell().id).not.toBe(makeCell().id);
  });
});
