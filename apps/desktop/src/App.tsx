import { useEffect, useLayoutEffect, useRef, useState } from "preact/hooks";
import { getCurrentWindow } from "@tauri-apps/api/window";
import { open, save, message as showMessage } from "@tauri-apps/plugin-dialog";
import { exists, readTextFile, writeTextFile } from "@tauri-apps/plugin-fs";
import { runNotebook } from "@abel/wasm";
import {
  makeCell,
  newNotebook,
  notebookPayload,
  parseNotebook,
  type Notebook,
} from "./notebook";
import "./App.css";

const readEditable = (element: HTMLDivElement) =>
  element.innerText.replace(/\r\n/g, "\n");

const notebookFilter = [{ name: "Abel Notebook", extensions: ["abelnb"] }];

const filenameFromPath = (path: string) =>
  path.split(/[\\/]/).filter(Boolean).pop() ?? "untitled.abelnb";

const withNotebookExtension = (path: string) =>
  path.toLowerCase().endsWith(".abelnb") ? path : `${path}.abelnb`;

function CodeInput({
  value,
  label,
  onChange,
  onFocus,
  onKeyDown,
}: {
  value: string;
  label: string;
  onChange: (value: string) => void;
  onFocus: () => void;
  onKeyDown: (event: KeyboardEvent) => void;
}) {
  const element = useRef<HTMLDivElement>(null);

  useLayoutEffect(() => {
    if (element.current && readEditable(element.current) !== value) {
      element.current.textContent = value;
    }
  }, [value]);

  return (
    <div
      ref={element}
      class="cell-input"
      role="textbox"
      aria-label={label}
      aria-multiline="true"
      contentEditable={true}
      spellcheck={false}
      onInput={(event) => onChange(readEditable(event.currentTarget))}
      onFocus={onFocus}
      onKeyDown={onKeyDown}
      onPaste={(event) => {
        event.preventDefault();
        const text = event.clipboardData?.getData("text/plain") ?? "";
        document.execCommand("insertText", false, text);
      }}
    />
  );
}

function Icon({ name }: { name: "plus" | "play" | "save" | "open" | "close" }) {
  if (name === "plus")
    return (
      <svg viewBox="0 0 16 16" aria-hidden="true">
        <path d="M8 3v10M3 8h10" />
      </svg>
    );
  if (name === "play")
    return (
      <svg viewBox="0 0 16 16" aria-hidden="true">
        <path d="m5 3 7 5-7 5V3Z" />
      </svg>
    );
  if (name === "save")
    return (
      <svg viewBox="0 0 16 16" aria-hidden="true">
        <path d="M3 2.5h8l2 2V13.5H3v-11ZM5 2.5v4h5v-4M5.5 13.5v-4h5v4" />
      </svg>
    );
  if (name === "open")
    return (
      <svg viewBox="0 0 16 16" aria-hidden="true">
        <path d="M2.5 4.5h4l1.2 1.5h5.8v7h-11v-8.5ZM2.5 6h11" />
      </svg>
    );
  return (
    <svg viewBox="0 0 16 16" aria-hidden="true">
      <path d="m4 4 8 8M12 4l-8 8" />
    </svg>
  );
}

function App() {
  const [notebook, setNotebook] = useState<Notebook>(newNotebook);
  const [selected, setSelected] = useState(0);
  const [running, setRunning] = useState(false);
  const [message, setMessage] = useState("Ready");
  const [filename, setFilename] = useState("untitled.abelnb");
  const [currentPath, setCurrentPath] = useState<string | null>(null);
  const [dirty, setDirty] = useState(false);
  const [persisting, setPersisting] = useState(false);
  const allowClose = useRef(false);
  const dirtyRef = useRef(dirty);
  const saveRef = useRef<() => Promise<boolean>>(async () => false);
  dirtyRef.current = dirty;

  useEffect(() => {
    const handleShortcut = (event: KeyboardEvent) => {
      if (!(event.metaKey || event.ctrlKey)) return;
      if (event.key === "s") {
        event.preventDefault();
        void saveNotebook();
      }
      if (event.key === "Enter") {
        event.preventDefault();
        void run();
      }
    };
    window.addEventListener("keydown", handleShortcut);
    return () => window.removeEventListener("keydown", handleShortcut);
  });

  const updateCell = (index: number, source: string) => {
    setNotebook((current) => ({
      ...current,
      cells: current.cells.map((cell, cellIndex) =>
        cellIndex < index
          ? cell
          : {
              ...cell,
              source: cellIndex === index ? source : cell.source,
              output: cellIndex === index ? "" : cell.output,
              value: cellIndex === index ? "" : cell.value,
              error: undefined,
              status: cellIndex >= index ? "stale" : cell.status,
            },
      ),
    }));
    setDirty(true);
    setMessage("Unsaved changes");
  };

  const run = async (endIndex = selected) => {
    setRunning(true);
    setMessage("Evaluating…");
    try {
      const results = await runNotebook(
        notebook.cells.slice(0, endIndex + 1).map((cell) => cell.source),
      );
      setNotebook((current) => ({
        ...current,
        cells: current.cells.map((cell, index) => {
          const result = results[index];
          if (!result)
            return index <= endIndex ? { ...cell, status: "stale" } : cell;
          return {
            ...cell,
            output: result.output ?? "",
            value: result.value ?? "",
            error: result.ok ? undefined : result.error,
            status: result.ok ? "success" : "error",
          };
        }),
      }));
      const failed = results.findIndex((result) => !result.ok);
      setMessage(
        failed === -1
          ? `Evaluated ${results.length} ${results.length === 1 ? "cell" : "cells"}`
          : `Cell ${failed + 1} needs attention`,
      );
    } catch (error) {
      setMessage(
        `Runtime unavailable: ${error instanceof Error ? error.message : String(error)}`,
      );
    } finally {
      setRunning(false);
    }
  };

  const addCell = (after = notebook.cells.length - 1) => {
    setNotebook((current) => ({
      ...current,
      cells: [
        ...current.cells.slice(0, after + 1),
        makeCell(),
        ...current.cells.slice(after + 1),
      ],
    }));
    setSelected(after + 1);
    setDirty(true);
  };
  const removeCell = (index: number) => {
    if (notebook.cells.length === 1) return;
    setNotebook((current) => ({
      ...current,
      cells: current.cells.filter((_, cellIndex) => cellIndex !== index),
    }));
    setSelected(Math.max(0, Math.min(selected, notebook.cells.length - 2)));
    setDirty(true);
  };

  const confirmDiscard = async () => {
    if (!dirty) return true;
    const choice = await showMessage("This notebook has unsaved changes.", {
      title: "Unsaved changes",
      kind: "warning",
      buttons: { yes: "Save", no: "Discard", cancel: "Cancel" },
    });
    if (choice === "yes") return await saveNotebook();
    return choice === "no";
  };

  const createNotebook = async () => {
    if (!(await confirmDiscard())) return;
    setNotebook(newNotebook());
    setFilename("untitled.abelnb");
    setCurrentPath(null);
    setSelected(0);
    setDirty(false);
    setMessage("New notebook");
  };

  const writeNotebook = async (path: string) => {
    try {
      setPersisting(true);
      setMessage("Saving…");
      await writeTextFile(path, notebookPayload(notebook));
      setCurrentPath(path);
      setFilename(filenameFromPath(path));
      setDirty(false);
      setMessage("Notebook saved");
      return true;
    } catch (error) {
      setMessage(
        `Could not save notebook: ${error instanceof Error ? error.message : String(error)}`,
      );
      return false;
    } finally {
      setPersisting(false);
    }
  };

  const saveNotebookAs = async () => {
    if (persisting) return false;
    try {
      const selectedPath = await save({
        title: "Save Abel Notebook",
        defaultPath: filename,
        filters: notebookFilter,
      });
      if (!selectedPath) {
        setMessage("Save cancelled");
        return false;
      }
      const path = withNotebookExtension(selectedPath);
      if (await exists(path)) {
        const overwrite = await showMessage(
          `Replace “${filenameFromPath(path)}”?`,
          {
            title: "File already exists",
            kind: "warning",
            buttons: "YesNo",
          },
        );
        if (overwrite !== "Yes") {
          setMessage("Save cancelled");
          return false;
        }
      }
      return await writeNotebook(path);
    } catch (error) {
      setMessage(
        `Could not save notebook: ${error instanceof Error ? error.message : String(error)}`,
      );
      return false;
    }
  };

  const saveNotebook = async () => {
    if (persisting) return false;
    if (!currentPath) return await saveNotebookAs();
    return await writeNotebook(currentPath);
  };
  saveRef.current = saveNotebook;

  const requestOpen = async () => {
    if (!(await confirmDiscard())) return;
    let selectedPath: string | string[] | null;
    try {
      selectedPath = await open({
        title: "Open Abel Notebook",
        multiple: false,
        filters: notebookFilter,
      });
    } catch (error) {
      setMessage(
        `Could not open notebook: ${error instanceof Error ? error.message : String(error)}`,
      );
      return;
    }
    if (!selectedPath || Array.isArray(selectedPath)) {
      setMessage("Open cancelled");
      return;
    }
    if (!selectedPath.toLowerCase().endsWith(".abelnb")) {
      setMessage("Could not open notebook: expected a .abelnb file");
      return;
    }
    try {
      setPersisting(true);
      setMessage("Opening…");
      const parsed = parseNotebook(await readTextFile(selectedPath));
      setNotebook(parsed);
      setFilename(filenameFromPath(selectedPath));
      setCurrentPath(selectedPath);
      setSelected(0);
      setDirty(false);
      setMessage("Notebook opened");
    } catch (error) {
      setMessage(
        `Could not open notebook: ${error instanceof Error ? error.message : String(error)}`,
      );
    } finally {
      setPersisting(false);
    }
  };

  useEffect(() => {
    const currentWindow = getCurrentWindow();
    let active = true;
    const unlisten = currentWindow.onCloseRequested(async (event) => {
      if (!active || allowClose.current || !dirtyRef.current) return;
      event.preventDefault();
      const choice = await showMessage("Save changes before closing?", {
        title: "Unsaved changes",
        kind: "warning",
        buttons: { yes: "Save", no: "Discard", cancel: "Cancel" },
      });
      if (choice === "cancel") return;
      if (choice === "yes" && !(await saveRef.current())) return;
      allowClose.current = true;
      await currentWindow.close();
    });
    return () => {
      active = false;
      void unlisten.then((dispose) => dispose());
    };
  }, []);
  const clearOutputs = () =>
    setNotebook((current) => ({
      ...current,
      cells: current.cells.map((cell) => ({
        ...cell,
        output: "",
        value: "",
        error: undefined,
        status: "idle",
      })),
    }));

  return (
    <main class="app-shell">
      <header class="topbar">
        <div class="brand">
          <span class="brand-mark">a</span>
          <span>abel</span>
          <span class="brand-divider" />
          <span class="brand-document">Notebook</span>
        </div>
        <nav class="toolbar" aria-label="Notebook actions">
          <button
            class="tool-button"
            onClick={() => void createNotebook()}
            disabled={persisting}
          >
            <Icon name="plus" />
            New
          </button>
          <button
            class="tool-button"
            onClick={() => void requestOpen()}
            disabled={persisting}
          >
            <Icon name="open" />
            Open
          </button>
          <button
            class="tool-button"
            onClick={() => void saveNotebook()}
            disabled={persisting}
          >
            <Icon name="save" />
            Save <kbd>⌘S</kbd>
          </button>
          <span class="toolbar-rule" />
          <button
            class="tool-button"
            onClick={() => void run()}
            disabled={running}
          >
            <Icon name="play" />
            Run cell <kbd>⌘↵</kbd>
          </button>
          <button
            class="run-all"
            onClick={() => void run(notebook.cells.length - 1)}
            disabled={running}
          >
            <Icon name="play" />
            Run all
          </button>
        </nav>
      </header>
      <div class="workspace">
        <section class="notebook" aria-label="Abel notebook">
          <div class="notebook-header">
            <div>
              <h1>{filename.replace(/\.abelnb$/, "")}</h1>
              <p>Interactive numerical computing</p>
            </div>
            <button class="quiet-button" onClick={clearOutputs}>
              Clear outputs
            </button>
          </div>
          <div class="cells">
            {notebook.cells.map((cell, index) => (
              <article
                class={`cell ${selected === index ? "active" : ""} ${cell.status}`}
                key={cell.id}
                onClick={() => setSelected(index)}
              >
                <div class="cell-gutter">
                  <span class="cell-number">In [{index + 1}]</span>
                  <span class="cell-state">
                    {cell.status === "stale"
                      ? "stale"
                      : cell.status === "error"
                        ? "error"
                        : cell.status === "success"
                          ? "ok"
                          : ""}
                  </span>
                </div>
                <div class="cell-main">
                  <CodeInput
                    value={cell.source}
                    label={`Input cell ${index + 1}`}
                    onChange={(value) => updateCell(index, value)}
                    onFocus={() => setSelected(index)}
                    onKeyDown={(event) => {
                      if (
                        (event.metaKey || event.ctrlKey) &&
                        event.key === "Enter"
                      ) {
                        event.preventDefault();
                        void run(index);
                      }
                    }}
                  />
                  {(cell.output || cell.value || cell.error) && (
                    <div
                      class={`cell-output ${cell.error ? "error-output" : ""}`}
                    >
                      <div class="output-label">Out [{index + 1}]</div>
                      <pre>
                        {cell.output.trimEnd()}
                        {cell.value && cell.value !== "null"
                          ? `${cell.output ? "\n" : ""}↳ ${cell.value}`
                          : ""}
                        {cell.error
                          ? `${cell.output ? "\n" : ""}${cell.error.category} error at ${cell.error.line}:${cell.error.column}\n${cell.error.message}`
                          : ""}
                      </pre>
                    </div>
                  )}
                </div>
                <div class="cell-actions">
                  <button
                    aria-label={`Add cell after ${index + 1}`}
                    onClick={(event) => {
                      event.stopPropagation();
                      addCell(index);
                    }}
                  >
                    <Icon name="plus" />
                  </button>
                  {notebook.cells.length > 1 && (
                    <button
                      aria-label={`Delete cell ${index + 1}`}
                      onClick={(event) => {
                        event.stopPropagation();
                        removeCell(index);
                      }}
                    >
                      <Icon name="close" />
                    </button>
                  )}
                </div>
              </article>
            ))}
            <button class="add-cell-prompt" onClick={() => addCell()}>
              <Icon name="plus" />
              Add a cell
            </button>
          </div>
          <footer class="statusbar">
            <span>{message}</span>
            <span>
              {notebook.cells.length}{" "}
              {notebook.cells.length === 1 ? "cell" : "cells"} · Abel notebook
            </span>
          </footer>
        </section>
      </div>
    </main>
  );
}

export default App;
