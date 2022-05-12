local testcase = require('testcase')
local errno = require('errno')
local mkdir = require('mkdir')
local fstat = require('fstat')

function testcase.after_each()
    local DIRS = {
        'testdir',
        'foo',
        'bar',
        'baz',
        'qux',
    }
    for i = #DIRS, 2, -1 do
        local dirname = table.concat(DIRS, '/', 1, i)
        os.remove(dirname)
    end

    os.remove('testdir/symdir/foo')
end

function testcase.mkdir()
    -- test that make a directory in the symbolic link directory
    local ok, err = mkdir('./testdir/foo')
    assert(ok, err)
    local stat = assert(fstat('./testdir/foo'))
    assert.equal(stat.type, 'directory')
    assert.equal(stat.perm, '0755')

    -- test that returns an error if non directory path segment already exist
    local f = assert(io.open('./testdir/foo/bar', 'w'))
    f:write('hello')
    f:close()
    ok, err = mkdir('./testdir/foo/bar')
    assert.is_false(ok)
    assert.equal(err.type, errno.EEXIST)

    -- test that return an error if pathname too long
    ok, err = mkdir('./testdir/' .. string.rep('a', 1000 * 1000))
    assert.is_false(ok)
    assert.equal(err.type, errno.ENAMETOOLONG)

    -- test that return an error if pathname contains an illegal byte sequence
    ok, err = mkdir('./testdir/' .. string.char(0) .. 'hello')
    assert.is_false(ok)
    assert.equal(err.type, errno.EILSEQ)

    -- test that returns an error if last path segment is invalid
    ok, err = mkdir('./testdir/..')
    assert.is_false(ok)
    assert.equal(err.type, errno.EINVAL)

    -- test that throws an error
    err = assert.throws(mkdir, {})
    assert.match(err, '#1 .+ [(]string expected, ', false)
end

function testcase.mkdir_with_mode()
    -- test that make a directory with specified mode
    local ok, err = mkdir('./testdir/foo', '0777')
    assert(ok, err)
    local stat = assert(fstat('./testdir/foo'))
    assert.equal(stat.type, 'directory')
    assert.equal(stat.perm, '0755')

    -- test that throws an error
    err = assert.throws(mkdir, './testdir/foo/bar', '0134777')
    assert.match(err, '#2 .+', false)

    err = assert.throws(mkdir, './testdir/foo/bar', 3071)
    assert.match(err, '#2 .+', false)

    err = assert.throws(mkdir, './testdir/foo/bar', {})
    assert.match(err, '#2 .+ [(]integer expected, ', false)
end

function testcase.mkdir_with_parent()
    -- test that make a directory and parent directories as needed
    local ok, err = mkdir('./testdir/foo/bar', nil, true)
    assert(ok, err)
    local stat = assert(fstat('./testdir/foo/bar'))
    assert.equal(stat.type, 'directory')
    assert.equal(stat.perm, '0755')

    -- test that returns an error if parent is not true
    ok, err = mkdir('./testdir/baz/qux')
    assert.is_false(ok)
    assert.equal(err.type, errno.ENOENT)

    -- test that throws an error
    err = assert.throws(mkdir, './testdir/hello/world', nil, {})
    assert.match(err, '#3 .+ [(]boolean expected, ', false)
end

function testcase.mkdir_with_follow_symlink()
    -- test that make a directory in the symbolic link directory
    local ok, err = mkdir('./testdir/symdir/foo')
    assert(ok, err)
    local stat = assert(fstat('./testdir/symdir/foo'))
    assert.equal(stat.perm, '0755')

    -- test that returns an error if set follow_symlink to false
    ok, err = mkdir('./testdir/symdir/bar', nil, nil, false)
    assert.is_false(ok)
    assert.equal(err.type, errno.EEXIST)

    -- test that throws an error
    err = assert.throws(mkdir, './testdir/foo', nil, nil, {})
    assert.match(err, '#4 .+ [(]boolean expected, ', false)
end

