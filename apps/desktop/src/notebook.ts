export type CellStatus = "idle" | "success" | "error" | "stale";
export type NotebookCell = {
  id: string;
  source: string;
  output: string;
  value: string;
  error?: { category: string; message: string; line: number; column: number };
  status: CellStatus;
};
export type Notebook = { format: 1; cells: NotebookCell[] };

export const starter = `fn mean(values) {
    return sum(values) / length(values)
}

scores = [72, 81, 95]
mean(scores)`;

export const makeCell = (source = ""): NotebookCell => ({
  id: crypto.randomUUID(),
  source,
  output: "",
  value: "",
  status: "idle",
});

export const newNotebook = (): Notebook => ({
  format: 1,
  cells: [makeCell(starter)],
});

export const notebookPayload = (notebook: Notebook) =>
  JSON.stringify(
    {
      format: 1,
      cells: notebook.cells.map(({ id, source }) => ({ id, source })),
    },
    null,
    2,
  );

export function parseNotebook(
  text: string,
  makeId: () => string = () => crypto.randomUUID(),
): Notebook {
  const parsed = JSON.parse(text) as Partial<Notebook>;
  if (
    parsed.format !== 1 ||
    !Array.isArray(parsed.cells) ||
    parsed.cells.length === 0
  )
    throw new Error("unsupported notebook format");
  return {
    format: 1,
    cells: parsed.cells.map((cell) => ({
      ...makeCell(),
      ...cell,
      id: typeof cell.id === "string" ? cell.id : makeId(),
      source: typeof cell.source === "string" ? cell.source : "",
      status: "stale",
      output: "",
      value: "",
    })),
  };
}
