# poplar-lang
Poplar, a very simple language for Agon

a small compiler, which translates Poplar code (.pplr) into a VM code file (.ppx)

a small VM which loads (.ppx) files aon Agon and runs them.

Instruction	Side Effect
push(n: f64);	Push a number onto the stack.
add();	Pop two numbers off of the stack, and push their sum.
subtract();	Pop two numbers off of the stack. Subtract the first from the second, and push the result.
multiply();	Pop two numbers off of the stack, and push their product.
divide();	Pop two numbers off of the stack. Divide the second by the first, and push the result.
sign();	Pop a number off of the stack. If it is greater or equal to zero, push 1, otherwise push -1.
allocate();	Pop a number off of the stack, and return a pointer to that number of free cells on the heap.
free();	Pop a number off of the stack, and go to where this number points in memory. Pop another number off of the stack, and free that many cells at this location in memory.
store(size: i32);	Pop a number off of the stack, and go to where this number points in memory. Then, pop size numbers off of the stack. Store these numbers in reverse order at this location in memory.
load(size: i32);	Pop a number off of the stack, and go to where this number points in memory. Then, push size number of consecutive memory cells onto the stack.
call(fn: i32);	Call a user defined function by it's compiler assigned ID.
call_foreign_fn(name: String);	Call a foreign function by its name in source.
begin_while();	Start a while loop. For each iteration, pop a number off of the stack. If the number is not zero, continue the loop.
end_while();	Mark the end of a while loop.
load_base_ptr();	Load the base pointer of the established stack frame, which is always less than or equal to the stack pointer. Variables are stored relative to the base pointer for each function. So, a function that defines x: num and y: num, x might be stored at base_ptr + 1, and y might be stored at base_ptr + 2. This allows functions to store variables in memory dynamically and as needed, rather than using static memory locations.
establish_stack_frame(arg_size: i32, local_scope_size: i32);	Pop off arg_size number of cells off of the stack and store them away. Then, call load_base_ptr to resume the parent stack frame when this function ends. Push local_scope_size number of zeroes onto the stack to make room for the function's variables. Finally, push the stored argument cells back onto the stack as they were originally ordered.
end_stack_frame(return_size: i32, local_scope_size: i32);	Pop off return_size number of cells off of the stack and store them away. Then, pop local_scope_size number of cells off of the stack to discard the stack frame's memory. Pop a value off of the stack and store it in the base pointer to resume the parent stack frame. Finally, push the stored return value cells back onto the stack as they were originally ordered.
