local unpack = unpack or table.unpack
local testcase = require('testcase')
local errno = require('error').errno
local mkdir = require('mkdir')
local fstat = require('fstat')

local DIRS = {
    'testdir',
    'foo',
    'bar',
    'baz',
    'qux',
}

function testcase.after_all()
    for i = #DIRS, 1, -1 do
        local dirname = table.concat(DIRS, '/', 1, i)
        os.remove(dirname)
    end
end

function testcase.mkdir()
    -- test that make a directory
    local ok, err, eno = mkdir('./testdir')
    assert(ok, err)
    assert.is_nil(eno)
    local stat = assert(fstat('./testdir'))
    assert.equal(stat.perm, '0755')

    -- test that make a directory and parent directories as needed
    ok, err, eno = mkdir('./testdir/foo/bar', nil, true)
    assert(ok, err)
    assert.is_nil(eno)

    -- test that make a directory with specified mode
    ok, err, eno = mkdir('./testdir/foo/bar/baz', '0777')
    assert(ok, err)
    assert.is_nil(eno)
    stat = assert(fstat('./testdir/foo/bar/baz'))
    assert.equal(stat.perm, '0755')

    -- test that return an error if pathname too long
    ok, err, eno = mkdir('./testdir/' .. string.rep('a', 1000 * 1000))
    assert.is_false(ok)
    assert.is_string(err)
    assert.equal(errno[eno], errno.ENAMETOOLONG)

    -- test that return an error if pathname contains an illegal byte sequence
    ok, err, eno = mkdir('./testdir/' .. string.char(0) .. 'hello')
    assert.is_false(ok)
    assert.is_string(err)
    assert.equal(errno[eno], errno.EILSEQ)

    -- test that returns an error if last path segment is invalid
    ok, err, eno = mkdir('./testdir/..')
    assert.is_false(ok)
    assert.is_string(err)
    assert.equal(errno[eno], errno.EINVAL)

    -- test that returns an error if non directory path segment already exist
    local f = assert(io.open('./testdir/foo/bar/baz/qux', 'w'))
    f:write('hello')
    f:close()
    ok, err, eno = mkdir('./testdir/foo/bar/baz/qux')
    assert.is_false(ok)
    assert.is_string(err)
    assert.equal(errno[eno], errno.EEXIST)

    -- test that throws an error
    for _, v in ipairs({
        {
            args = {},
            match_err = '#1 .+ [(]string expected, ',
        },
        {
            args = {
                './testdir',
                '0134777',
            },
            match_err = '#2 .+',
        },
        {
            args = {
                './testdir',
                3071,
            },
            match_err = '#2 .+',
        },
        {
            args = {
                './testdir',
                {},
            },
            match_err = '#2 .+ [(]number expected, ',
        },
        {
            args = {
                './testdir',
                nil,
                1,
            },
            match_err = '#3 .+ [(]boolean expected, ',
        },
    }) do
        err = assert.throws(mkdir, unpack(v.args))
        assert.match(err, v.match_err, false)
    end
end

