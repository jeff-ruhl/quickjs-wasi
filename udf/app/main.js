(function() {
    "use strict";

    function sqrt(x) {
        return x ** 0.5;
    }

    let math = {
        sqrt: sqrt	
    };

    globalThis['math'] = math;
})();
