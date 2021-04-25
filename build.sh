emcc TaskList.c --pre-js TaskList.Interface.js -s WASM=1 -s FETCH=1 -s RESERVED_FUNCTION_POINTERS=2 -s EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap", "ccall", "addFunction", "UTF8ToString"]'  -o TaskList.js



