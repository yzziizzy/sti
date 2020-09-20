

; comment

struct car
	pos f32 3
	vel f32 3
	acc f32 3
	
	name string
end

frame
local bar s64
local baz s64
local end s64
set bar 13
set baz 2
set end 5

label foo
; sub bar baz bar
args bar
call func
sub bar baz bar

cond bar > end
goto foo
halt


label func
frame
stack_dump
unframe
ret

