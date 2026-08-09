print("before open")
local tty2, err = io.open("/dev/tty2", "w")
print("after open", tty2, err)
assert(tty2, err)

tty2:write("hello from lua on tty2\n")
tty2:flush()
tty2:close()

print("done")