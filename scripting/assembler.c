// Public Domain

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "assembler.h"


void terrible_interpreter(Context* ctx);




// shitty but easy
int op_name_lookup(char* name) {
	if(0) {}
	#define IT(n, x, y) else if(0 == strcmp(#n, name)) return IT_##n;
		IT_CROWD
	#undef IT
	else return 0;
}










static void stack_pushl(Context* ctx, int64_t v) {
	*((int64_t*)(ctx->stack + ctx->stackHead)) = v;
	ctx->stackHead += sizeof(v);
}

static uint64_t stack_popl(Context* ctx) {
	if(ctx->stackHead == 0) {
		printf("stack underflow\n");
		exit(1);
	}
	ctx->stackHead -= sizeof(int64_t);
	int64_t v = *((int64_t*)(ctx->stack + ctx->stackHead));
	return v;
}


static FrameInfo* get_frame(Context* ctx, int n) {
	if(n >= VEC_LEN(&ctx->frames)) return NULL;
// 	printf("getting frame %d\n", VEC_LEN(&ctx->frames) - n - 1);
	return &VEC_ITEM(&ctx->frames, VEC_LEN(&ctx->frames) - n - 1);
}

static FrameInfo* add_frame(Context* ctx) {
	
	stack_pushl(ctx, ctx->stackBase);
	ctx->stackBase = ctx->stackHead;
	
	VEC_INC(&ctx->frames);
	
	FrameInfo* f = get_frame(ctx, 0);
// 	HT_init(&f->labels, 32);
	HT_init(&f->locals, 32);
	
	return f;
}

static void free_frame(Context* ctx) {
	FrameInfo* f = get_frame(ctx, 0);
	
	ctx->stackHead = ctx->stackBase;
	ctx->stackBase = stack_popl(ctx);
	
// 	HT_destroy(&f->labels, 0);
	HT_destroy(&f->locals, 1);
	
	VEC_LEN(&ctx->frames)--;
}

static void add_label(Context* ctx, char* name, size_t index) {
// 	FrameInfo* f = get_frame(ctx, 0);
	
	HT_set(&ctx->labels, name, index);
	printf("adding label '%s' at %ld\n", name, index);
}

static size_t get_label(Context* ctx, char* name) {
	size_t out;
// 	FrameInfo* f = get_frame(ctx, 0);

	if(HT_get(&ctx->labels, name, &out)) {
		printf("unknown label: '%s'\n", name);
		return 0;
	}
	
	return out;
}

static size_t get_local_offset(Context* ctx, int frame, char* name) {
	LocalInfo* local;
	FrameInfo* f = get_frame(ctx, frame);
	
	if(HT_get(&f->locals, name, &local)) {
		printf("unknown local: '%s'\n", name);
		return 0;
	}
	
	return local->offset;
}

static int64_t get_stackl(Context* ctx, size_t off) {
	return *((int64_t*)(ctx->stack + off));
}

static void set_stackl(Context* ctx, size_t off, int64_t v) {
	*((int64_t*)(ctx->stack + off)) = v;
}

static int64_t get_locall(Context* ctx, char* name) {
	return get_stackl(ctx, get_local_offset(ctx, 0, name));
}

static void set_locall(Context* ctx, char* name, int64_t v) {
	set_stackl(ctx, get_local_offset(ctx, 0, name), v);
}


static void add_local(Context* ctx, char* name) {
	FrameInfo* f = get_frame(ctx, 0);
	
	LocalInfo* local = calloc(1, sizeof(*local));
	local->offset = ctx->stackHead;
	local->name = strdup(name);
	
	ctx->stackHead += sizeof(int64_t);
	
	HT_set(&f->locals, name, local);
	printf("adding local '%s'\n", name);
}

static void check_stack(Context* ctx, size_t extra) {
	if(ctx->stackAlloc < ctx->stackHead + extra) {
		ctx->stackAlloc += extra;
		ctx->stack = realloc(ctx->stack, ctx->stackAlloc);
	}
}



void run_inst(Context* ctx) {
	Inst* in;
	size_t p;
	int64_t va, vb, vc;
	
	if(ctx->ip >= ctx->instLen) {
		printf("Instruction pointer out of bounds\n");
		ctx->halt = 1;
		return;
	}  
	
	in = &ctx->inst[ctx->ip]; 
	
	switch(in->opid) {
		case IT_label: 
			add_label(ctx, in->args[0], ctx->ip + 1);
			break;
		
		case IT_halt:
			printf("halting.\n");
			ctx->halt = 1;
			return;
			
		case IT_goto: // direct jump to a label
			// find label, jump to it.
			printf("jumping to label %s\n", in->args[0]);
			p = get_label(ctx, in->args[0]);
			ctx->ip = p;
			return;
			
		case IT_call: // function call
			// find label, jump to it.
			printf("calling function %s\n", in->args[0]);
			
			stack_pushl(ctx, ctx->ip + 1);
			p = get_label(ctx, in->args[0]);
			ctx->ip = p;
			return;
			
		case IT_ret: // 
			printf("returning\n");
			ctx->ip = stack_popl(ctx);
			return;
			
		case IT_frame: // start a new stack frame
			// push base pointer
			add_frame(ctx);
			break;
		
		case IT_unframe: // roll back tothe previous stack frame
			// push base pointer
			free_frame(ctx);
			break;
			/*
		case IT_args: // push some args
			
			for(int i = 0; i < in->argc; i++) {
				stack_pushl(ctx, get_locall(ctx, in->args[i]));
			}
			
			// push the size of the arguments
			stack_pushl(ctx, in->argc * 4);
			
			break;
		
		case IT_unargs: // pop off the most recent argument stack
			va = stack_pop1(ctx);
			ctx->stackHead -= va;
			break;
			*/
			
		case IT_local: // declare a local variable
			// arg1: name
			// arg2: type
			// arg3: width
			add_local(ctx, in->args[0]/*, in->args[1], in->args[2]*/);
			
// 			t->name = in->args[0];
// 			t->type = vartype_from_name(in->args[1]);
// 			t->width = strtol(in->args[2], NULL, 10);
			
			break;
		
		case IT_debug:
			printf("Debug instruction: %s\n", in->args[0]);
			break;
		
		case IT_stack_dump:
			printf("Stack dump instruction:\n");
			
			FrameInfo* f;
			for(int i = 0; f = get_frame(ctx, i); i++) {
				printf("  frame %d:\n", i);
				
				HT_LOOP(&f->locals, n, LocalInfo*, l) {
					printf("    %s: %ld\n", l->name, get_stackl(ctx, l->offset));
				}
			}
			
			break;
		
		case IT_set:
			printf("setting local %s to %ld\n", in->args[0], strtol(in->args[1], NULL, 10));
			set_locall(ctx, in->args[0], strtol(in->args[1], NULL, 10));
			break;
			
		case IT_add:
			printf("adding locals: %s + %s = %s\n", in->args[0], in->args[1], in->args[2]);
			
			va = get_locall(ctx, in->args[0]);
			vb = get_locall(ctx, in->args[1]);
			set_locall(ctx, in->args[2], va + vb);
			break;
			
		case IT_sub:
			printf("subtracting locals: %s + %s = %s\n", in->args[0], in->args[1], in->args[2]);
			
			va = get_locall(ctx, in->args[0]);
			vb = get_locall(ctx, in->args[1]);
			set_locall(ctx, in->args[2], va - vb);
			break;
		
		
		case IT_cond: // skip next instruciton if false
			va = get_locall(ctx, in->args[1]);
			vb = get_locall(ctx, in->args[2]);
			
			if(0 == strcmp(">", in->args[0])) {
				if(!(va > vb)) ctx->ip++;
			}
			else if(0 == strcmp("<", in->args[0])) {
				if(!(va < vb)) ctx->ip++;
			}
			else if(0 == strcmp("<=", in->args[0])) {
				if(!(va <= vb)) ctx->ip++;
			}
			else if(0 == strcmp(">=", in->args[0])) {
				if(!(va >= vb)) ctx->ip++;
			}
			else if(0 == strcmp("==", in->args[0])) {
				if(!(va == vb)) ctx->ip++;
			}
			
			
			break;
		
	
	}
	
	
	printf("\n");
	
	ctx->ip++;
	return;
	
}



void terrible_interpreter(Context* ctx) {
	
	
	
	
	for(int i = 0; i < 40; i++) {
		if(ctx->halt) break;
		
		printf("%d - ", i);
		run_inst(ctx);
	}
	
}








static void push_inst(Context* ctx, Inst inst) {
	if(ctx->instLen <= ctx->instAlloc) {
		ctx->instAlloc *= 2;
		ctx->inst = realloc(ctx->inst, ctx->instAlloc * sizeof(*ctx->inst));
	}
	
	ctx->inst[ctx->instLen++] = inst;
}


int main(int argc, char* argv[]) {
	size_t len, numLines;
	char* source;
	char** lines;
	Lexer* ls;
	
	if(argc < 2) {
		fprintf(stderr, "Error: expected asm file as first argument.\n");
		return 1;
	}
	
	source = readWholeFile(argv[1], &len);
	
	
	Context ctx;
	
	VEC_INIT(&ctx.frames);
	HT_init(&ctx.labels, 64);
	ctx.inst = calloc(1, sizeof(*ctx.inst) * 64);
	ctx.instAlloc = 64;
	ctx.instLen = 0;
	ctx.ip = 0;
	ctx.halt = 0;
	ctx.stack = calloc(1, 1024);
	ctx.stackAlloc = 1024;
	ctx.stackBase = 0;
	ctx.stackHead = 0;
	
	
	Inst in;
	int n = 0;
	
	ls = start_lexer(source, len);
	while(next_token(ls)) {
		printf("TOKEN: %s - '%.*s'\n", lexer_state_names[ls->tokenState], ls->blen, ls->buffer);
		
		
		if(ls->tokenState == LST_endl) {
			push_inst(&ctx, in);
			n = 0;
			
			continue;
		}
		
		
		if(n == 0) {
			in.opid = op_name_lookup(ls->buffer);
			
		}
		else {
			in.args[in.argc] = strdup(ls->buffer);
			in.argc++;
		}
		
		n++;
	}
	
	
	// scan and fill in labels
// 	for(size_t i = 0; i < instLen; i++) {
// 		if(inst[i].opid != IT_label) continue;
// 		add_label(ctx, inst[i].args[0], i);
// 	}
	
// 	terrible_interpreter(VEC_DATA(&inst), VEC_LEN(&inst));
	
	
	
	terrible_interpreter(&ctx);
	
	return 0;
}






