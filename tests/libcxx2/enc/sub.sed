s/^[ \t]*\.globl[ \t]*\<[A-Za-z_][A-Za-z_0-9]*\>/# &/g
s/^#[ \t]*\.globl[ \t]*\<main\>/.globl main/g
s/\<main\>/__main__/g
