#!/usr/bin/lua

local function test(t)
    local dataf = io.open("data.txt", "wb")
    dataf:write(t.data)
    dataf:close()
    os.execute(string.format("./iterator data.txt %s %s > output.txt", t.sig, t.cmd))
    local outputf = io.open("output.txt", "rb")
    local got = outputf:read("*a")
    outputf:close()
    local expected = table.concat(t, '\n') .. "\n"
    if expected == got then
        print("Test", t.sig, "PASS")
    else
        print("Test", t.sig, "FAIL")
        print()
        print("Expected")
        print(expected)
        print()
        print("Got")
        print(got)
        print()
        print()
    end
end

test {
    cmd = "y",
    sig = "y",
    data = "\002",
    "DATA 'y' 1",
    "ITER 'y' 0/1",
    "U8 2",
    "ITER '' 1/1",
}

test {
    cmd = 'q',
    sig = 'q',
    data = "\002\001",
    "DATA 'q' 2",
    "ITER 'q' 0/2",
    "U16 258",
    "ITER '' 2/2"
}

test {
    cmd = 'n',
    sig = 'n',
    data = "\002\001",
    "DATA 'n' 2",
    "ITER 'n' 0/2",
    "I16 258",
    "ITER '' 2/2"
}

test {
    cmd = 'n',
    sig = 'n',
    data = "\254\254",
    "DATA 'n' 2",
    "ITER 'n' 0/2",
    "I16 -258",
    "ITER '' 2/2"
}

