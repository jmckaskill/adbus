#!/usr/bin/lua

local function test(t)
    local dataf = io.open("data.txt", "wb")
    dataf:write(t.data)
    dataf:close()
    os.execute(string.format("./iterator data.txt %s %s > output.txt", t.sig, t.cmd))
    local outputf = io.open("output.txt", "rb")
    local got = outputf:read("*a")
    outputf:close()
    local expected = table.concat(t, '\n')
    if output2 == output then
        print("Pass:", t.sig)
    else
        print("Fail:", t.sig)
        print("Expected:", expected)
        print("Got:", got)
    end
end

test {
    cmd = "y",
    sig = "y",
    data = "\002",
    "DATA 'y' 1",
    "ITER 'y' 0/1",
    "U8 0 2",
    "ITER '' 1/0",
}

test {
    cmd = 'q',
    sig = 'q',
    data = "\002\001",
    "DATA 'q' 2",
    "ITER 'q' 0/2",
    "U16 0 258",
    "ITER '' 2/0"
}

