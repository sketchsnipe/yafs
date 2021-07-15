/* yafs - yet another forth shell */

/* License CC0: 

 To the greatest extent permitted by, but not in contravention
 of, applicable law, Affirmer hereby overtly, fully, permanently,
 irrevocably and unconditionally waives, abandons, and surrenders all of
 Affirmer's Copyright and Related Rights (patents, trademark etc.)
 and associated claims and causes of action, whether now known or unknown
 (including existing as well as future claims and causes of action),
 in the Work (i) in all territories worldwide, (ii) for the maximum
 duration provided by applicable law or treaty (including future time
 extensions), (iii) in any current or future medium and for any number
 of copies, and (iv) for any purpose whatsoever, including without
 limitation commercial, advertising or promotional purposes.

******************************************************************************/

#include <stdlib.h> /* Needed for exit(); */
#include <stdio.h> /* Needed for putchar */
#include <string.h> /* Needed for strlen */

#define MEM_SIZE 10000
int mem[MEM_SIZE];

/* Memory map
|----- TOP -----|_mem[MEM_SIZE]
|		|
|    Stack	|
|_______________|_mem[STACK]
|		|
| Return Stack	|
|_______________|_mem[RSTACK]
|		|
|  Dictionary	|	// This is array of pointers to all the dictionary entries
|  Address	|
|  Stack	|
|_______________|_mem[DICT]
|		|
|		|
|		|
|  Dictionary	|
|  Memory	|
|		|
|		|
|_______________|_mem[DICT_MEM]
|		|
|   Pointers	|
|		|
|--- BOTTOM ----|_mem[PTR]
*/

#define STACK_SIZE 100
#define RETURN_STACK_SIZE 100
#define DICTIONARY_SIZE 1000
#define NUMBER_OF_POINTERS 4

/* locations */

int const STACK =	MEM_SIZE - STACK_SIZE;
int const RSTACK =	MEM_SIZE - STACK_SIZE - RETURN_STACK_SIZE;
int const DSTACK = 	MEM_SIZE - STACK_SIZE - RETURN_STACK_SIZE - DICTIONARY_SIZE;
int const DICT_MEM =	NUMBER_OF_POINTERS;
int const PTR =		0;

/* pointers */

#define stack_ptr	mem[0]	// Points to the top of the stack
#define rstack_ptr	mem[1]	// Points to the top of the return stack
#define dstack_ptr	mem[2]	// Points to the latest dictionary address stack entry
#define dict_mem_ptr	mem[3]	// Points to the top (last free space)  of the dictionary memory
/* 	...
	bruh_ptr	mem[NUMBER_OF_POINTERS] */

void ptr_init(void)
{
	stack_ptr = STACK;	
	rstack_ptr = RSTACK;	
	dstack_ptr = DSTACK;	
	dict_mem_ptr = DICT_MEM;
}

/* __ first functions __
*
* stack
* 	iterate_stack_ptr()		__memory safe removal from stack
* 	degradate_stack_ptr()		__memory safe adding to stack
*
* rstack
* 	iterate_rstack_ptr()		__memory safe removal from rstack
* 	degradate_rstack_ptr()		__memory safe adding to rstack
* 	
* dict	
* 	iterate_dict_stack_ptr()	__memory safe adding to dictionary address stack
*	iterate_dict_mem_ptr()		__memory safe adding to dictionary memory
*	dict_mem_write(data, addr)	__memory safe write to dictionary memory space
*/ 	


/* stack */

void iterate_stack_ptr (void)
{
	if(stack_ptr < MEM_SIZE)
	{
		stack_ptr = stack_ptr + 1;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');
	}
}

void degradate_stack_ptr (void)
{
	if(STACK < stack_ptr)
	{
		stack_ptr = stack_ptr - 1;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');
	}
}


/* rstack */

void iterate_rstack_ptr (void)
{
	if(rstack_ptr < STACK)
	{
		stack_ptr = stack_ptr + 1;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');
	}
}

void degradate_rstack_ptr (void)
{
	if(RSTACK < rstack_ptr)
	{
		stack_ptr = stack_ptr - 1;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');
	}
}


/* dict */

void iterate_dstack_ptr (void)
{
	if(dstack_ptr < RSTACK)
	{
		dstack_ptr = stack_ptr - 1;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');
	}
}

void iterate_dict_mem_ptr (int amount)
{
	if(dict_mem_ptr < DSTACK)
	{
		dstack_ptr = dstack_ptr + amount;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');
	}
}

void dict_mem_write (int data)
{
	if(DICT_MEM < dict_mem_ptr && dict_mem_ptr < DSTACK)
	{
		mem[dict_mem_ptr] = data;
	}
	else
	{
		putchar('F');	/* Rip */
		putchar('\n');

		exit(1);	/* if the dict memory overflows, just end the program */
	}
}


/* __ base functions __
*
* stack
* 	push(x)		__pushes int x onto stack
* 	pop()		__pops x off stack and returns it
*
* rstack	
* 	rpush(x)	__pushes addr x onto stack
* 	rpop()		__pops addr x off stack and returns it
*
* dict		__Read "Dictionary structure" comment to understand this bit
*	dpush(x)	__pushes an address onto the dictionary address stack
* 	define(		__puts a word in the dictionary and adds its addr to the dict stack
*		char* word,
*		int immediate,
*		void (*f)(void *), 
*		char* data
*	      )
*/ 	


/* stack */

void push(int val)
{
	iterate_stack_ptr();
	mem[stack_ptr] = val;
}

int pop(void)
{
	int val = mem[stack_ptr];
	degradate_stack_ptr();
	return val;
}


/* rstack */

void rpush(int val)
{
	iterate_rstack_ptr();
	mem[rstack_ptr] = val;
}

int rpop(void)
{
	int val = mem[rstack_ptr];
	degradate_rstack_ptr();
	return val;
}


/* dict */

void dpush(int val)
{
	iterate_dstack_ptr();
	mem[dstack_ptr] = val;
}

/* This is for reading and writing the function pointer into mem[] */
#define	FP_READ(fp, addr)	memcpy(&fp, &mem[addr], sizeof(fp))
#define	FP_WRITE(fp, addr)	memcpy(&mem[addr], &fp, sizeof(fp))
#define FP_SIZE			sizeof(void(*)())/sizeof(int)

void define(
		char* word,		/* string, null terminated already */
		int immediate,
		int hidden,
		void (*fp)(void *),
		char* data		/* also a null terminated string */
				)
{	
	/* push the current dict_mem_ptr location onto the dictionary stack */
	dpush(dict_mem_ptr);


	/* NOTE: dict_mem_write() writes to the dict_mem_ptr location */


	/* write the word */
	int word_size = strlen(word); /* i could have this pointer shit wrong */
	for(int i = 0; i < word_size + 1; i++) 	/* word_size + 1 because we want the */
	{					/* null (\0) character written too */
		dict_mem_write(word[i]);
		iterate_dict_mem_ptr(1);
	}

	/* write the immediate flag */
	dict_mem_write(immediate);
	iterate_dict_mem_ptr(1);

	/* write the hidden flag */
	dict_mem_write(hidden);
	iterate_dict_mem_ptr(1);

	/* write the function pointer */
	FP_WRITE(fp, dict_mem_ptr);
	iterate_dict_mem_ptr(FP_SIZE);


	/* write the data string 	-- NOTE: builtins will just have a null character here (\0) */
	int data_size = strlen(data); /* i could still have this pointer shit wrong */
	for(int i = 0; i < data_size + 1; i++) 	/* word_size + 1 because we want the */
	{					/* null (\0) character written too */
		dict_mem_write(data[i]);
		iterate_dict_mem_ptr(1);
	}
}

/* Dictionary structure
* 
* word			string (null-terminated)
* immediate flag	int
* hidden flag		int
* function pointer  <--------------------------------------(* for interprited words, this calls the interiter
* data		       	string (null-terminated)	   (* for builtins, this calls a function directly
*/ 

/* __ Builtins__
* 
* stack
*	drop	(a -- ) __ deletes the top of the stack
* 
 */

/* stack */

void drop(void)
{
	degradate_stack_ptr();
}
/* pointer */
void (*drop_ptr)(void) = &drop;


void echo(void)
{
	putchar(pop());
}
/* pointer */
void (*echo_ptr)(void) = &echo;



/* Forth */

void builtin_init(void)
{
	
}

void interpriter(void)
{
	
}

int main(void)
{
	ptr_init();
	return 0;
}
