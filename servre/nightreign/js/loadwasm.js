var canvasElement = document.getElementById('canvas');

// As a default initial behavior, pop up an alert when webgl context is lost. To make your
// application robust, you may want to override this behavior before shipping!
// See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
canvasElement.addEventListener('webglcontextlost', (e) => {
    alert('WebGL context lost. You will need to reload the page.');
    e.preventDefault();
}, false);

var Module = {
    canvas: canvasElement,
    setStatus: function(text) {
        console.log("status: " + text);
    }
};
window.onerror = function(event) {
    console.log("onerror: " + event);
};

var xhr = new XMLHttpRequest();
xhr.open('GET', 'EMTest.wasm', true)
xhr.responseType = 'arraybuffer';
xhr.onload = function () {
    Module.wasmBinary = xhr.response;
    var script = document.createElement('script');
    script.src = "EMTest.js";
    document.body.appendChild(script);
};
xhr.send(null);