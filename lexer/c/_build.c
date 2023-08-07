// gcc -lutil build.c -o hacer && ./hacer

#include "_build.inc.c"

// link with -lutil


char* sources[] = { "test.c",
	"cpp.c", 
	"lexer.c", 
	"parser.c", 
	"parser_typedec.c", 
	"sti/sti.c", 
	NULL,
};

// these are run through pkg-config
char* libs_needed[] = {
	NULL,
};

char* ld_add[] = {
	"-lm",
	NULL,
};


// -ffast-math but without reciprocal approximations 
char* cflags[] = {
	"-std=gnu11", 
	"-ggdb", 
	"-ffunction-sections", "-fdata-sections",
	"-DLINUX",
	"-march=native",
	"-mtune=native", 
	"-DSTI_C3DLAS_NO_CONFLICT",
	"-fno-math-errno", 
	"-fexcess-precision=fast", 
	"-fno-signed-zeros",
	"-fno-trapping-math", 
	"-fassociative-math", 
	"-ffinite-math-only", 
	"-fno-rounding-math", 
	"-fno-signaling-nans", 
	"-include signal.h", 
	"-pthread", 
	"-Wall", 
	"-Werror", 
	"-Wextra", 
	"-Wno-unused-result", 
	"-Wno-unused-variable", 
	"-Wno-unused-but-set-variable", 
	"-Wno-unused-function", 
	"-Wno-unused-label", 
	"-Wno-unused-parameter",
	"-Wno-pointer-sign", 
	"-Wno-missing-braces", 
	"-Wno-maybe-uninitialized", 
	"-Wno-implicit-fallthrough", 
	"-Wno-sign-compare", 
	"-Wno-char-subscripts", 
	"-Wno-int-conversion", 
	"-Wno-int-to-pointer-cast", 
	"-Wno-unknown-pragmas",
	"-Wno-sequence-point",
	"-Wno-switch",
	"-Wno-parentheses",
	"-Wno-comment",
	"-Wno-strict-aliasing",
	"-Wno-endif-labels",
	"-Werror=implicit-function-declaration",
	"-Werror=uninitialized",
	"-Werror=return-type",
	NULL,
};



int compile_source(char* src_path, char* obj_path) {
	char* cmd = sprintfdup("gcc -c -o %s %s %s", obj_path, src_path, g_gcc_opts_flat);
//	printf("%s\n", cmd);
	strlist_push(&compile_cache, cmd);
//	exit(1);
	return 0;
}



void check_source(char* raw_src_path, strlist* objs) {
	time_t src_mtime, obj_mtime = 0, dep_mtime = 0;
	
	char* src_path = resolve_path(raw_src_path, &src_mtime);
	char* src_dir = dir_name(raw_src_path);
	char* base = base_name(src_path);
	
	char* build_dir = path_join("build", src_dir);
	char* obj_path = path_join(build_dir, base);
	
	// cheap and dirty
	size_t olen = strlen(obj_path);
	obj_path[olen-1] = 'o';
	
	
	strlist_push(objs, obj_path);
	
	char* dep_path = strcatdup(build_dir, "/", base, ".d");
	
	mkdirp_cached(build_dir, 0755);
	
	char* real_obj_path = resolve_path(obj_path, &obj_mtime);
	if(obj_mtime < src_mtime) {
//		printf("  objtime compile\n");
		compile_source(src_path, real_obj_path);
		return;
	}
	
	
	if(gen_deps(src_path, dep_path, src_mtime, obj_mtime)) {
//		printf("  deep dep compile\n");
		compile_source(src_path, real_obj_path);
	}
	
	
	
	//gcc -c -o $2 $1 $CFLAGS $LDADD
}



int main(int argc, char* argv[]) {
	string_cache_init(2048);
	realname_cache_init();
	strlist_init(&compile_cache);
	hash_init(&mkdir_cache, 128);
	g_nprocs = get_nprocs();
	
	char* tmp;
	
	char* exe_path = "lextest";
	unlink(exe_path);
	
	mkdirp_cached("build", 0755);
	
	g_gcc_opts_list = concat_lists(ld_add, cflags);
	g_gcc_opts_flat = join_str_list(g_gcc_opts_list, " ");
	g_gcc_include = pkg_config(libs_needed, "I");
	g_gcc_libs = pkg_config(libs_needed, "L");
	tmp = g_gcc_opts_flat;
	g_gcc_opts_flat = strjoin(" ", g_gcc_opts_flat, g_gcc_include);
	free(tmp);
	
//	rglob src;
	//recursive_glob("src", "*.[ch]", 0, &src);
	
	strlist objs;
	strlist_init(&objs);
	
	float source_count = list_len(sources);
	
	for(int i = 0; sources[i]; i++) {
//		printf("%i: checking %s\n", i, sources[i]);
		check_source(sources[i], &objs);
		
		printf("\rChecking dependencies...  %s", printpct((i * 100) / source_count));
	}
	printf("\rChecking dependencies...  \e[32mDONE\e[0m\n");
	fflush(stdout);
	
	
	
	if(compile_cache_execute()) {
		printf("\e[1;31mBuild failed.\e[0m\n");
		return 1;
	}
	
	char* objects_flat = join_str_list(objs.entries, " ");
	
	
	printf("Creating archive...      "); fflush(stdout);
	if(system(sprintfdup("ar rcs build/tmp.a %s", objects_flat))) {
		printf(" \e[1;31mFAIL\e[0m\n");
		return 1;
	}
	else {
		printf(" \e[32mDONE\e[0m\n");
	}
	
	
	printf("Linking executable...    "); fflush(stdout);
	char* cmd = sprintfdup("gcc -Wl,--gc-sections build/tmp.a -o %s %s %s", exe_path, g_gcc_libs, g_gcc_opts_flat);
	if(system(cmd)) {
		printf(" \e[1;31mFAIL\e[0m\n");
		return 1;
	}
	else {
		printf(" \e[32mDONE\e[0m\n");
	}
	
	// erase the build output if it succeeded
	printf("\e[F\e[K");
	printf("\e[F\e[K");
	printf("\e[F\e[K");
	printf("\e[F\e[K");
	
	printf("\e[32mBuild successful:\e[0m %s\n\n", exe_path);
	
	return 0;
}


