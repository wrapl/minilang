function create(tag) {
	let classes = tag.split(".");
	let element = document.createElement(classes[0]);
	for (var i = 1; i < classes.length; ++i) element.classList.add(classes[i]);

	function process(arg) {
		if (arg === null) {
		} else if (arg instanceof Array) {
			arg.forEach(process);
		} else if (arg instanceof Element) {
			element.appendChild(arg);
		} else if (typeof arg === "string") {
			element.appendChild(document.createTextNode(arg));
		} else if (typeof arg === "object") {
			for (var key in arg) {
				let value = arg[key];
				if (key.startsWith("on-")) {
					element.addEventListener(key.substring(3), value);
				} else if (key.startsWith("prop-")) {
					element.setProperty(key.substring(5), value);
				} else {
					element.setAttribute(key, value);
				}
			}
		}
	}
	for (var j = 1; j < arguments.length; ++j) process(arguments[j]);
	return element;
}

let sessions = [];
	
function ml_output(index, text) {
	sessions[index].output(text);
}

function ml_finish(index) {
	sessions[index].finish();
}

Module.onRuntimeInitialized = function() {
	let ml_session = Module.cwrap("ml_session", "number", []);
	let ml_session_evaluate = Module.cwrap("ml_session_evaluate", "number", ["number", "string"]);
	
	function Cell(session, value) {
		this.session = session;
		let input = this.input = create("textarea");
		if (value) input.value = value;
		let output = this.output = create("p");
		// Font Awesome Free 6.6.0 by @fontawesome - https://fontawesome.com License - https://fontawesome.com/license/free Copyright 2024 Fonticons, Inc.
		let play = create("svg", {xmlns: "http://www.w3.org/2000/svg", viewBox: "0 0 384 512", style: "width:1em;height:1em;"},
			create("path", {d: "M73 39c-14.8-9.1-33.4-9.4-48.5-.9S0 62.6 0 80L0 432c0 17.4 9.4 33.4 24.5 41.9s33.7 8.1 48.5-.9L361 297c14.3-8.7 23-24.2 23-41s-8.7-32.2-23-41L73 39z"})
		)
		let element = create("div.cell",
			create("div.input", input, create("button", {"on-click": evaluate.bind(this)}, play)),
			create("div.output", output)
		);
		session.cells.push(this);
		session.element.appendChild(element);
	
		function evaluate() {
			session.evaluate(this, input.value);
		}
	}
	
	function Session() {
		this.index = ml_session();
		sessions[this.index] = this;
		this.element = create("div.session");
		let cells = this.cells = [];
		if (arguments.length > 0) {
			for (var i = 0; i < arguments.length; ++i) {
				new Cell(this, arguments[i]);
			}
		} else {
			new Cell(this, "");
		}
		this.active = null;
		cells[0].input.focus();
	}
	
	Session.prototype.evaluate = function(cell, text) {
		this.active = cell;
		cell.output.textContent = "";
		ml_session_evaluate(this.index, text);
	}
	
	Session.prototype.output = function(text) {
		console.log("output", text);
		this.active.output.textContent += text;
	}
	
	Session.prototype.finish = function() {
		console.log("finish");
		//this.active = null;
		let n = this.cells.indexOf(this.active);
		if (n + 1 >= this.cells.length) {
			new Cell(this, "").input.focus();
		} else {
			this.cells[n + 1].input.focus();
		}
	}
	
	let nodes = document.querySelectorAll(".tryit")
	for (let i = 0; i < nodes.length; ++i) {
		let node = nodes[i];
		let code = [];
		let items = node.children;
		for (let j = 0; j < items.length; ++j) {
			code.push(items[j].textContent);
		}
		while (items.length) node.removeChild(items[0]);
		let session = new Session(...code);
		node.appendChild(session.element);
	}
}
