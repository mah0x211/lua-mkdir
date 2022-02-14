# lua-mkdir

[![test](https://github.com/mah0x211/lua-mkdir/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-mkdir/actions/workflows/test.yml)

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


## ok, err, errno = mkdir( pathname [, mode [, parents [, follow_symlink]]] )

make directories.

**Parameters**

- `pathname:string`: path of the directory to create.
- `mode:string|integer`: file permissions in octal notation as a string, or integer. (default: `'0777'`)
- `parents:boolean`: make parent directories as needed. (default: `false`)
- `follow_symlink:boolean`: follow symbolic links. (default: `true`)

**Returns**

- `ok:boolean`: `true` on success.
- `err:string`: error message.
- `errno:integer`: error number.
