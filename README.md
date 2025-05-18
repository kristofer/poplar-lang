# poplar-lang
Poplar, a very simple language for Agon

a small compiler, which translates Poplar code (.pplr) into a VM code file (.ppx)

a small VM which loads (.ppx) files aon Agon and runs them.

### Data types:

- Char (a byte)
- String (a array of (n:i16 bytes)
- Int (int-16)
- Ptr (int-24) used for address space

internal data structures for the VM: 
a vmStack, and a vmTemp stack (with limited static memory 256 bytes?) and a vmHeap

there is an outputBuffer (outb) which is where code can place bytes to sent to standard out. (puts())

there is an inChar where the latest kbd input char(byte) is placed when the kdb interrupts. (inchar())

## Instructions	-> Logic & Results

DRAFT 1

### vmStack Ops

- PUSHN **push(n: i16);**	Push a literal number onto the stack.

### Binary Ops

- ADD **add();**	Pop two numbers off of the stack, and push their sum.
- SUB **subtract();**	Pop two numbers off of the stack. Subtract the first from the second, and push the result.
- MUL **multiply();**	Pop two numbers off of the stack, and push their product.
- DIV **divide();**	Pop two numbers off of the stack. Divide the second by the first, and push the result.
- MOD **modulo();** Pop two numbers off of the stack. Mod the second by the first and push the result.

- **sign();**	Pop a number off of the stack. If it is greater or equal to zero, push 1, otherwise push -1.

### Dynamic mem ops

- **allocate();**	Pop int off of the stack, and return a pointer to that number of free bytes on the heap.
- **free();**	Pop a pointer off of the stack. Pop number off of the stack, and free that many bytes at pointer location in memory.

### The loop
- **begin_while();**	Start a while loop. For each iteration, pop a number off of the stack. If the number is not zero, run the loop.
- **end_while();**	Mark the end of a while loop.

### Memory ops

- **store(size: i24);**	Pop a pointer off of the stack. Then, pop size bytes off of the stack. Store these bytes in reverse order at the memory pointer.
- **load(size: i24);**	Pop a pointer off of the stack, and push size onto stack. Then, push size number of consecutive memory bytes onto the stack.

- **call(fn: i16);**	Call a user defined function by it's compiler assigned ID.
- //call_foreign_fn(name: String);	Call a foreign function by its name in source.
- **load_frame_ptr();**	Load the frame pointer of the current stack frame, which is always less than or equal to the stack pointer. Variables are stored relative to the frame pointer for each function. 
So, a function that defines x: num and y: num, x might be stored at base_ptr + 1, and y might be stored at base_ptr + 2. 
This allows functions to store variables in memory dynamically on the vmStack and as needed.

- **make_stack_frame(arg_size: i8, local_scope_size: i8);**	Pop off arg_size number of cells off of the stack and push them to the vmTemp stack. 
Then, call load_base_ptr to resume the parent stack frame when this function ends. 
Push local_scope_size number of zeroes onto the stack to make room for the function's variables. 
Finally, push the stored argument cells back onto the stack as they were originally ordered from the vmTemp stack.
- **drop_stack_frame(return_size: i8, local_scope_size: i8);**	Pop off return_size number of bytes off of the stack and push them to the vmTemp stack. 
Then, pop/drop local_scope_size number of cells off of the stack to discard the stack frame's memory. 
Pop a value off of the stack and store it in the base pointer to resume the parent stack frame. 
Finally, push the stored return value cells back onto the stack as they were originally ordered.
