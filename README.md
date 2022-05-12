# lua-mkdir

[![test](https://github.com/mah0x211/lua-mkdir/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-mkdir/actions/workflows/test.yml)
[![codecov](https://codecov.io/gh/mah0x211/lua-mkdir/branch/master/graph/badge.svg)](https://codecov.io/gh/mah0x211/lua-mkdir)

make directories.


## Installation

```
luarocks install mkdir
```

## Usage

```lua
local mkdir = require('mkdir')
assert(mkdir('./hello/world/dir', '0777', true))
```

## Error Handling

the following functions return the `error` object created by https://github.com/mah0x211/lua-errno module.


## ok, err = mkdir( pathname [, mode [, parents [, follow_symlink]]] )

make directories.

**Parameters**

- `pathname:string`: path of the directory to create.
- `mode:string|integer`: file permissions in octal notation as a string, or integer. (default: `'0777'`)
- `parents:boolean`: make parent directories as needed. (default: `false`)
- `follow_symlink:boolean`: follow symbolic links. (default: `true`)

**Returns**

- `ok:boolean`: `true` on success.
- `err:error`: error object.
