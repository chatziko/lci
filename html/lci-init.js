var statusElement = document.getElementById('status');
var progressElement = document.getElementById('progress');
var spinnerElement = document.getElementById('spinner');

// Proper blocking input is not possible in emscripten. So we do the input locally via the
// xterm-local-echo plugin, and feed it to lci via the Module.readLine function.

var term = new Terminal();
var localEcho = new LocalEchoController(null, {
    historySize: 1000,		// the history mechanism is not working well, so we pass 1000 here, but only keep 100 entries in localStorage
});
var fitAddon = new FitAddon.FitAddon();

// load saved history
try {
	var hist = localStorage.getItem("history");
	localEcho.history.entries = hist ? JSON.parse(hist) : [];
} catch(e) {
	localEcho.history.entries = [];
}
localEcho.history.cursor = localEcho.history.entries.length;

term.loadAddon(localEcho);
term.loadAddon(fitAddon);

term.open(document.getElementById('terminal'));
term.focus();
fitAddon.fit();

term.onData((data) => {
	if(data.charCodeAt(0) == 3)	// 3 = Ctrl-C
		Module._ctrlCPressed = true;
});

var Module = {
	// Called asynchronously (via asyncify) from lci to get a line of input.
	readLine: async function() {
		var line = await localEcho.read("lci> ");

		// save last 100 entries of history in localStorage
		localStorage.setItem("history", JSON.stringify(localEcho.history.entries.slice(-100)));

		var lengthBytes = lengthBytesUTF8(line) + 1;
		var stringOnWasmHeap = _malloc(lengthBytes);
		stringToUTF8(line, stringOnWasmHeap, lengthBytes);
		return stringOnWasmHeap;
	},

	// Called asynchronously (via asyncify) from lci to get a line of input.
	readChar: async function() {
		// localEcho.readChar() is buggy, so we do it manually
		return new Promise(resolve => {
			var disp = term.onData(data => {
				disp.dispose();
				resolve(data.charCodeAt(0));
			});
		});
	},

	_ctrlCPressed: false,
	readCtrlC: async function() {
		// Give the browser some responsiveness, and allow Ctrl-C to be registered.
		await new Promise(resolve => setTimeout(resolve, 10));

		var res = Module._ctrlCPressed;
		Module._ctrlCPressed = false;
		return res;
	},

	stdout: function(output) {
		// xterm.js wants utf8 data as Uint8Array
		if(output == 10)
			term.write(Uint8Array.from([13]));

		term.write(Uint8Array.from([output]));
	},

	preRun: [],
	postRun: [],
	print: (function() {
		return function(text) {
			if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
			console.log("print ", text);
		};
	})(),
	printErr: function(text) {
		if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
		console.error(text);
	},
	setStatus: function(text) {
		if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
		if (text === Module.setStatus.last.text) return;
		var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
		var now = Date.now();
		if (m && now - Module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
		Module.setStatus.last.time = now;
		Module.setStatus.last.text = text;
		if (m) {
			text = m[1];
			progressElement.value = parseInt(m[2])*100;
			progressElement.max = parseInt(m[4])*100;
			progressElement.hidden = false;
			spinnerElement.hidden = false;
		} else {
			progressElement.value = null;
			progressElement.max = null;
			progressElement.hidden = true;
			if (!text) spinnerElement.hidden = true;
		}
		statusElement.innerHTML = text;
	},
	totalDependencies: 0,
	monitorRunDependencies: function(left) {
		this.totalDependencies = Math.max(this.totalDependencies, left);
		Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
	},
};

Module.setStatus('Downloading...');
window.onerror = function() {
	Module.setStatus('Exception thrown, see JavaScript console');
	spinnerElement.style.display = 'none';
	Module.setStatus = function(text) {
		if (text) Module.printErr('[post-exception status] ' + text);
	};
};