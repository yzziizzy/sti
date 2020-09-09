

; comment


frame
local bar int
local baz int
local end int
set bar 13
set baz 2
set end 5

label foo
; sub bar baz bar
call func
stack_dump
cond > bar end
goto foo
halt


label func
sub bar baz bar
ret

