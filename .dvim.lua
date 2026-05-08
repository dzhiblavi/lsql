local merge = require("tools").merge
local home = os.getenv("HOME")
local c = require("config").default
local p = c.plugins
c.editor = merge(c.editor, {
    indent = 4,
    column_limit = 120,
    format_on_save = false,
})

p.debug.cc = {
    default_adapter = "codelldb",
    adapters = {
        codelldb = {
            command = home .. "/.local/share/nvim/mason/bin/codelldb",
        },
    },
}

c.lsp.servers.clangd.cmd = {
    "/opt/homebrew/opt/llvm@20/bin/clangd",
    "--background-index=true",
    "--clang-tidy=true",
    "--all-scopes-completion",
    "--offset-encoding=utf-16",
    "--completion-style=detailed",
    "--pch-storage=memory",
    "--header-insertion=never",
    "--log=error",
    "--pretty",
    "-j=8",
}

return c
