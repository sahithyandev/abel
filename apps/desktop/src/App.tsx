import { useEffect, useRef, useState } from "preact/hooks";
import { runNotebook } from "@abel/wasm";
import "./App.css";

type CellStatus = "idle" | "success" | "error" | "stale";
type Cell = { id: string; source: string; output: string; value: string; error?: { category: string; message: string; line: number; column: number }; status: CellStatus };
type Notebook = { format: 1; cells: Cell[] };
const starter = `fn mean(values) {
    return sum(values) / length(values)
}

scores = [72, 81, 95]
mean(scores)`;
const makeCell = (source = ""): Cell => ({ id: crypto.randomUUID(), source, output: "", value: "", status: "idle" });
const newNotebook = (): Notebook => ({ format: 1, cells: [makeCell(starter)] });

function Icon({ name }: { name: "plus" | "play" | "save" | "open" | "close" }) {
  if (name === "plus") return <svg viewBox="0 0 16 16" aria-hidden="true"><path d="M8 3v10M3 8h10" /></svg>;
  if (name === "play") return <svg viewBox="0 0 16 16" aria-hidden="true"><path d="m5 3 7 5-7 5V3Z" /></svg>;
  if (name === "save") return <svg viewBox="0 0 16 16" aria-hidden="true"><path d="M3 2.5h8l2 2V13.5H3v-11ZM5 2.5v4h5v-4M5.5 13.5v-4h5v4" /></svg>;
  if (name === "open") return <svg viewBox="0 0 16 16" aria-hidden="true"><path d="M2.5 4.5h4l1.2 1.5h5.8v7h-11v-8.5ZM2.5 6h11" /></svg>;
  return <svg viewBox="0 0 16 16" aria-hidden="true"><path d="m4 4 8 8M12 4l-8 8" /></svg>;
}

function App() {
  const [notebook, setNotebook] = useState<Notebook>(newNotebook);
  const [selected, setSelected] = useState(0);
  const [running, setRunning] = useState(false);
  const [message, setMessage] = useState("Ready");
  const [filename, setFilename] = useState("untitled.abelnb");
  const [dirty, setDirty] = useState(false);
  const fileInput = useRef<HTMLInputElement>(null);

  useEffect(() => {
    const handleShortcut = (event: KeyboardEvent) => {
      if (!(event.metaKey || event.ctrlKey)) return;
      if (event.key === "s") { event.preventDefault(); saveNotebook(); }
      if (event.key === "Enter") { event.preventDefault(); void run(); }
    };
    window.addEventListener("keydown", handleShortcut);
    return () => window.removeEventListener("keydown", handleShortcut);
  });

  const updateCell = (index: number, source: string) => {
    setNotebook((current) => ({ ...current, cells: current.cells.map((cell, cellIndex) => cellIndex < index ? cell : { ...cell, source: cellIndex === index ? source : cell.source, output: cellIndex === index ? "" : cell.output, value: cellIndex === index ? "" : cell.value, error: undefined, status: cellIndex >= index ? "stale" : cell.status }) }));
    setDirty(true);
    setMessage("Unsaved changes");
  };

  const run = async (endIndex = selected) => {
    setRunning(true); setMessage("Evaluating…");
    try {
      const results = await runNotebook(notebook.cells.slice(0, endIndex + 1).map((cell) => cell.source));
      setNotebook((current) => ({ ...current, cells: current.cells.map((cell, index) => { const result = results[index]; if (!result) return index <= endIndex ? { ...cell, status: "stale" } : cell; return { ...cell, output: result.output ?? "", value: result.value ?? "", error: result.ok ? undefined : result.error, status: result.ok ? "success" : "error" }; }) }));
      const failed = results.findIndex((result) => !result.ok);
      setMessage(failed === -1 ? `Evaluated ${results.length} ${results.length === 1 ? "cell" : "cells"}` : `Cell ${failed + 1} needs attention`);
    } catch (error) { setMessage(`Runtime unavailable: ${error instanceof Error ? error.message : String(error)}`); }
    finally { setRunning(false); }
  };

  const addCell = (after = notebook.cells.length - 1) => { setNotebook((current) => ({ ...current, cells: [...current.cells.slice(0, after + 1), makeCell(), ...current.cells.slice(after + 1)] })); setSelected(after + 1); setDirty(true); };
  const removeCell = (index: number) => { if (notebook.cells.length === 1) return; setNotebook((current) => ({ ...current, cells: current.cells.filter((_, cellIndex) => cellIndex !== index) })); setSelected(Math.max(0, Math.min(selected, notebook.cells.length - 2))); setDirty(true); };

  const canDiscard = () => !dirty || window.confirm("Discard unsaved notebook changes?");
  const createNotebook = () => { if (!canDiscard()) return; setNotebook(newNotebook()); setFilename("untitled.abelnb"); setSelected(0); setDirty(false); setMessage("New notebook"); };
  const requestOpen = () => { if (canDiscard()) fileInput.current?.click(); };

  const openNotebook = (event: Event) => {
    const input = event.currentTarget as HTMLInputElement; const file = input.files?.[0]; if (!file) return;
    const reader = new FileReader(); reader.onload = () => { try { const parsed = JSON.parse(String(reader.result)) as Notebook; if (parsed.format !== 1 || !Array.isArray(parsed.cells) || parsed.cells.length === 0) throw new Error("unsupported notebook format"); setNotebook({ format: 1, cells: parsed.cells.map((cell) => ({ ...makeCell(), ...cell, status: "stale", output: "", value: "" })) }); setFilename(file.name); setSelected(0); setDirty(false); setMessage("Notebook opened"); } catch (error) { setMessage(`Could not open notebook: ${error instanceof Error ? error.message : String(error)}`); } }; reader.readAsText(file); input.value = "";
  };
  const saveNotebook = () => { const payload = JSON.stringify({ format: 1, cells: notebook.cells.map(({ id, source }) => ({ id, source })) }, null, 2); const link = document.createElement("a"); link.href = URL.createObjectURL(new Blob([payload], { type: "application/json" })); link.download = filename; link.click(); URL.revokeObjectURL(link.href); setDirty(false); setMessage("Notebook saved"); };
  const clearOutputs = () => setNotebook((current) => ({ ...current, cells: current.cells.map((cell) => ({ ...cell, output: "", value: "", error: undefined, status: "idle" })) }));

  return <main class="app-shell">
    <header class="topbar"><div class="brand"><span class="brand-mark">a</span><span>abel</span><span class="brand-divider" /><span class="brand-document">Notebook</span></div><nav class="toolbar" aria-label="Notebook actions"><button class="tool-button" onClick={createNotebook}><Icon name="plus" />New</button><button class="tool-button" onClick={requestOpen}><Icon name="open" />Open</button><button class="tool-button" onClick={saveNotebook}><Icon name="save" />Save <kbd>⌘S</kbd></button><span class="toolbar-rule" /><button class="tool-button" onClick={() => void run()} disabled={running}><Icon name="play" />Run cell <kbd>⌘↵</kbd></button><button class="run-all" onClick={() => void run(notebook.cells.length - 1)} disabled={running}><Icon name="play" />Run all</button></nav><input ref={fileInput} class="visually-hidden" type="file" accept=".abelnb,.json" onChange={openNotebook} /></header>
    <div class="workspace"><aside class="sidebar"><div class="sidebar-heading"><span>Document</span><span class="live-dot" /><span class="saved-label">{dirty ? "Edited" : "Ready"}</span></div><div class="document-row"><span class="document-icon">∴</span><span class="document-name">{filename}</span></div><div class="sidebar-section"><span>Cells</span><span>{notebook.cells.length}</span></div><div class="cell-outline">{notebook.cells.map((cell, index) => <button class={`outline-row ${selected === index ? "selected" : ""}`} onClick={() => setSelected(index)} key={cell.id}><span class={`status-dot ${cell.status}`} /><span>In [{index + 1}]</span></button>)}</div><button class="sidebar-add" onClick={() => addCell()}><Icon name="plus" />Add cell</button><div class="sidebar-footer"><span>Abel 0.1</span><span class="runtime-state">WASM runtime</span></div></aside>
      <section class="notebook" aria-label="Abel notebook"><div class="notebook-header"><div><h1>{filename.replace(/\.abelnb$/, "")}</h1><p>Interactive numerical computing</p></div><button class="quiet-button" onClick={clearOutputs}>Clear outputs</button></div><div class="cells">{notebook.cells.map((cell, index) => <article class={`cell ${selected === index ? "active" : ""} ${cell.status}`} key={cell.id} onClick={() => setSelected(index)}><div class="cell-gutter"><span class="cell-number">In [{index + 1}]</span><span class="cell-state">{cell.status === "stale" ? "stale" : cell.status === "error" ? "error" : cell.status === "success" ? "ok" : ""}</span></div><div class="cell-main"><textarea aria-label={`Input cell ${index + 1}`} value={cell.source} spellcheck={false} onInput={(event) => updateCell(index, event.currentTarget.value)} onFocus={() => setSelected(index)} onKeyDown={(event) => { if ((event.metaKey || event.ctrlKey) && event.key === "Enter") { event.preventDefault(); void run(index); } }} />{(cell.output || cell.value || cell.error) && <div class={`cell-output ${cell.error ? "error-output" : ""}`}><div class="output-label">Out [{index + 1}]</div><pre>{cell.output.trimEnd()}{cell.value && cell.value !== "null" ? `${cell.output ? "\n" : ""}↳ ${cell.value}` : ""}{cell.error ? `${cell.output ? "\n" : ""}${cell.error.category} error at ${cell.error.line}:${cell.error.column}\n${cell.error.message}` : ""}</pre></div>}</div><div class="cell-actions"><button aria-label={`Add cell after ${index + 1}`} onClick={(event) => { event.stopPropagation(); addCell(index); }}><Icon name="plus" /></button>{notebook.cells.length > 1 && <button aria-label={`Delete cell ${index + 1}`} onClick={(event) => { event.stopPropagation(); removeCell(index); }}><Icon name="close" /></button>}</div></article>)}<button class="add-cell-prompt" onClick={() => addCell()}><Icon name="plus" />Add a cell</button></div><footer class="statusbar"><span>{message}</span><span>{notebook.cells.length} {notebook.cells.length === 1 ? "cell" : "cells"} · Abel notebook</span></footer></section></div>
  </main>;
}

export default App;
