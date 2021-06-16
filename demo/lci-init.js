var statusElement = document.getElementById('status');
var progressElement = document.getElementById('progress');
var spinnerElement = document.getElementById('spinner');

// When lci reads from stdin, emscripten calls Module.stdin, which however needs to be synchronous!
// Workaround: lci does an async call to Module.waitForInput (before trying to read from stdin), which finishes when
// a full line of input is read from xterm.js. The line is stored in termInput, and Module.stdin can feed it
// to lci synchronously when lci tries to read from stdin.
//
// Note that replxx in lci detects that it has no tty and does no output at all (no "lci >" prompt and no
// echo of characters). So we do the prompt/echo locally via the xterm-local-echo plugin.

var term = new Terminal();
var localEcho = new LocalEchoController();
var fitAddon = new FitAddon.FitAddon();

term.loadAddon(localEcho);
term.loadAddon(fitAddon);

term.open(document.getElementById('terminal'));
fitAddon.fit();

var termInput = "";

var Module = {
	// This waits until we have a line of input to give to lci.
	waitForInput: async function() {
		termInput = await localEcho.read("lci> ");
		termInput += "\n";
	},

	stdin: function() {
		if(termInput == "") {
			// console.log("returning null");
			return null;	// EOF
		}

		var res = termInput.charCodeAt(0);
		if(res == 13)
			res = 10;

		termInput = termInput.slice(1);

		// console.log("returning ", res);
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