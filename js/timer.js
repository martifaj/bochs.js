var LibraryTimer = {
    set_timer_js: function() {
        function tmpfunc(msec) {
            setTimeout(function () {
                var cpu_halted = _cpu_loop_js();
                if (cpu_halted) {
                    msec = 0; //1
                } else {
                    msec = 0;
                }
                tmpfunc(msec);
            }, msec);
        }
        tmpfunc();
    },
};
mergeInto(LibraryManager.library, LibraryTimer);
