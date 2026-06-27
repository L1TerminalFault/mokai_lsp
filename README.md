# MOKAI LSP

## Language server for Mokai Configuration TOML `mokai.toml`

### Built by **MOKAI** itself

#### Sorry for now the lsp only supports NeoVim! (and no automatic installation)

### To get started

## Add this snippet to your NeoVim config (most likely `~/.config/nvim/init.lua`)

```lua
vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  pattern = "mokai.toml",
  callback = function(ev)
    vim.lsp.start({
      name = "MokaiLSP",
      cmd = { "/home/xdk/Projects/mokai_lsp/build/debug/mokai_lsp" },
      root_dir = vim.fs.dirname(ev.match),
      settings = {},
    })
  end,
})
```
