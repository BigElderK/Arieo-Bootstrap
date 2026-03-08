// pre.js — executes on the main browser thread before Emscripten initialises.
//
// With -sPROXY_TO_PTHREAD=1 the linker replaces main() with a trampoline:
//   1. The trampoline runs on the main browser thread (window is available).
//   2. It reads Module['arguments'] and copies them into argv.
//   3. It creates a pthread and calls the ORIGINAL main(argc, argv) on it.
//
// The original main() runs on a Web Worker where window is undefined, so
// window.location.search cannot be read there.  We capture the ?manifest= query
// param here — on the main thread — and expose it as Module['arguments'] so the
// trampoline will forward it to main() as argv[1].

if (typeof Module === 'undefined') { var Module = {}; }

if (typeof window !== 'undefined') {
    var _params = new URLSearchParams(window.location.search);
    var _manifest = _params.get('manifest');
    if (_manifest) {
        Module['arguments'] = ['--manifest=' + _manifest];
    }
}
