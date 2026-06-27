# MOKAI LSP

## Language server for Mokai Configuration TOML `mokai.toml`

### Built by **MOKAI** itself

#### Sorry for now the lsp only supports NeoVim and only Linux! (and no automatic installation)

### To get started

**NOTICE** This program is useless if you don't use Mokai (the build engine)!

## Clone this repo, build with MOKAI

#### OR

## Clone this repo

# then

### Run `chmod +x ./mokai_lsp`

## Add this snippet to your NeoVim config (most likely `~/.config/nvim/init.lua`)

```lua
vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  pattern = "mokai.toml",
  callback = function(ev)
    vim.lsp.start({
      name = "MokaiLSP",
      cmd = { "<current_dir>/mokai_lsp" },
      root_dir = vim.fs.dirname(ev.match),
      settings = {},
    })
  end,
})
```

### Now you can use NeoVim with Mokai Build system and your mokai.toml will have the lsp support
