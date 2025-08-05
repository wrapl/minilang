from docutils.parsers.rst import Directive, roles
from docutils import nodes
import re
from pygments import lexers
import inspect
import os
from sphinx import version_info

class FoldersDirective(Directive):
	has_content = True

	def run(self):
		env = self.state.document.settings.env
		block = nodes.line_block()
		block['classes'].append('folders')
		node = nodes.bullet_list()
		block.append(node)
		stack = [node]
		indents = [-1]
		for line in self.content:
			match = re.search('[-+] ', line)
			if match is None:
				break
			indent = match.start()
			node = nodes.list_item()
			text = line[match.end():]
			while indent <= indents[-1]:
				stack.pop()
				indents.pop()
			stack[-1].append(node)
			if line[match.start()] == '+':
				node.append(nodes.inline(text = '📁 ' + text))
				node['classes'].append('folder')
				children = nodes.bullet_list()
				node.append(children)
				stack.append(children)
				indents.append(indent)
			else:
				node.append(nodes.inline(text = '🖹 ' + text))
				node['classes'].append('file')
		return [block]


class TryItDirective(Directive):
	has_content = True

	def run(self):
		env = self.state.document.settings.env
		block = nodes.line_block()
		block['classes'].append('tryit')
		text = ""
		print(self.content)
		for line in self.content:
			if line == "":
				node = nodes.list_item()
				node.append(nodes.inline(text = text.strip()))
				block.append(node)
				text = ""
			text += line + "\n"
		node = nodes.list_item()
		node.append(nodes.inline(text = text.strip()))
		block.append(node)
		return [block]


# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'Minilang'
copyright = '2019, Raja Mukherji'
author = 'Raja Mukherji'
github_url = "https://github.com/wrapl/minilang"

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
	'sphinx.ext.graphviz',
	'sphinx.ext.viewcode',
	'sphinx_toolbox.collapse',
	'sphinxcontrib.ansi',
	'breathe',
	'linuxdoc.rstFlatTable'
	#"sphinxawesome_theme"
	#'sphinx_design'
]

breathe_projects = {"minilang": "doxygen/xml"}

breathe_default_project = "minilang"

graphviz_output_format = "svg"

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

html_theme = 'insipid'

html_context = {
    'display_github': True,
    'github_user': 'wrapl',
    'github_repo': 'minilang',
}

html_theme_options = {
	"body_max_width": None,
	"globaltoc_collapse": True,
	"globaltoc_includehidden": True,
	"extra_header_links": {
		"gitHub": {
			"link": github_url,
			"icon": (
				"""<svg fill="currentColor" height="26px" style="margin-top:-2px;display:inline" viewBox="0 0 45 44" xmlns="http://www.w3.org/2000/svg">
					<path clip-rule="evenodd" d="M22.477.927C10.485.927.76 10.65.76 22.647c0 9.596 6.223 17.736 14.853 20.608 1.087.2 1.483-.47 1.483-1.047
					 0-.516-.019-1.881-.03-3.693-6.04 1.312-7.315-2.912-7.315-2.912-.988-2.51-2.412-3.178-2.412-3.178-1.972-1.346.149-1.32.149-1.32 2.18.154
					 3.327 2.24 3.327 2.24 1.937 3.318 5.084 2.36 6.321 1.803.197-1.403.759-2.36 1.379-2.903-4.823-.548-9.894-2.412-9.894-10.734
					 0-2.37.847-4.31 2.236-5.828-.224-.55-.969-2.759.214-5.748 0 0 1.822-.584 5.972 2.226 1.732-.482 3.59-.722 5.437-.732 1.845.01 3.703.25
					 5.437.732 4.147-2.81 5.967-2.226 5.967-2.226 1.185 2.99.44 5.198.217 5.748 1.392 1.517 2.232 3.457 2.232 5.828 0 8.344-5.078 10.18-9.916
					 10.717.779.67 1.474 1.996 1.474 4.021 0 2.904-.027 5.247-.027 5.96 0 .58.392 1.256 1.493 1.044C37.981 40.375 44.2 32.24 44.2
					 22.647c0-11.996-9.726-21.72-21.722-21.72" fill="currentColor" fill-rule="evenodd"></path>
				</svg>"""
			)
		}
	}
}

html_copy_source = False

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
#html_theme = "pydata_sphinx_theme"
# html_theme_options = {
# 	"collapse_navigation": True,
# 	"page_sidebar_items": [],
# 	"icon_links": [{
# 		"name": "GitHub",
# 		"url": "https://github.com/wrapl/minilang",
# 		"icon": "fab fa-github-square",
# 	}],
# 	"pygment_light_style": "minilang.MiniStyle",
# 	"pygment_dark_style": "minilang.MiniStyle"
# }
# html_sidebars = {
# 	"**": ["search-field", "page-toc", "sidebar-nav-bs"]
# }

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_css_files = [
	'css/custom.css',
]

html_js_files = [
    'js/minilang.js',
    'js/custom.js',
]

cautodoc_root = os.path.abspath('../src')

master_doc = 'index'

pygments_style = "minilang.MiniStyle"

rst_prolog = """
.. role:: mini(code)
   :language: mini
   :class: highlight

.. role:: html(code)
   :language: html
   :class: highlight

.. role:: c(code)
   :language: c
   :class: highlight

.. role:: json(code)
   :language: json
   :class: highlight
"""

def setup(sphinx):
	import sys, os
	sys.path.insert(0, os.path.abspath('./_util'))
	from minilang import MinilangLexer, minilangDomain
	if version_info[0] >= 4:
		sphinx.add_lexer("mini", MinilangLexer)
	else:
		sphinx.add_lexer("mini", MinilangLexer())
	lexers.LEXERS['mini'] = ('minilang', 'Minilang', ('mini',), ('*.mini', '*.rabs'), ('text/x-mini',))
	#sphinx.add_domain(minilangDomain)
	sphinx.add_directive('folders', FoldersDirective)
	sphinx.add_directive('tryit', TryItDirective)
	sphinx.add_css_file('css/custom.css')
