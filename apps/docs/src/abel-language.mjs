/**
 * TextMate grammar for Abel v0.1.
 *
 * Keeping this as a small grammar (rather than borrowing a C-like grammar)
 * means the docs highlight only syntax Abel actually defines.
 */
export default {
  name: "abel",
  displayName: "Abel",
  scopeName: "source.abel",
  fileTypes: ["abel"],
  patterns: [
    { include: "#comments" },
    { include: "#strings" },
    { include: "#numbers" },
    { include: "#functions" },
    { include: "#keywords" },
    { include: "#constants" },
    { include: "#builtins" },
    { include: "#operators" },
    { include: "#punctuation" },
  ],
  repository: {
    comments: {
      patterns: [
        {
          match: "//.*$",
          name: "comment.line.double-slash.abel",
        },
      ],
    },
    strings: {
      patterns: [
        {
          begin: '"',
          end: '"',
          name: "string.quoted.double.abel",
          patterns: [
            {
              match: '\\\\(?:\\\\|"|n)',
              name: "constant.character.escape.abel",
            },
          ],
        },
      ],
    },
    numbers: {
      patterns: [
        {
          match: "\\b\\d+(?:\\.\\d+)?\\b",
          name: "constant.numeric.abel",
        },
      ],
    },
    keywords: {
      patterns: [
        {
          match: "\\b(?:and|else|for|fn|if|in|not|or|return|while)\\b",
          name: "keyword.control.abel",
        },
      ],
    },
    constants: {
      patterns: [
        {
          match: "\\b(?:true|false|null)\\b",
          name: "constant.language.abel",
        },
      ],
    },
    builtins: {
      patterns: [
        {
          match: "\\b(?:print|length|sum|min|max|abs|sqrt)(?=\\s*\\()",
          name: "support.function.abel",
        },
      ],
    },
    functions: {
      patterns: [
        {
          match: "\\b(fn)\\s+([A-Za-z_]\\w*)",
          captures: {
            1: { name: "keyword.control.abel" },
            2: { name: "entity.name.function.abel" },
          },
        },
      ],
    },
    operators: {
      patterns: [
        {
          match: "==|!=|<=|>=|[+*/%<>=-]",
          name: "keyword.operator.abel",
        },
      ],
    },
    punctuation: {
      patterns: [
        {
          match: "[{}\\[\\](),;]",
          name: "punctuation.separator.abel",
        },
      ],
    },
  },
};
