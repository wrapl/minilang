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
		block_quote = nodes.line_block()
		block_quote['classes'].append('folders')
		node = nodes.bullet_list()
		block_quote.append(node)
		stack = [node]
		indents = [-1]
		for line in self.content:
			match = re.search('[-+] ', line)
			if match is None:
				break
			indent = match.start()
			node = nodes.list_item()
			text = line[match.end():]
			print(line[match.end():])
			if indent <= indents[-1]:
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
		return [block_quote]


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


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
	'sphinx.ext.graphviz',
	'sphinx.ext.viewcode',
	'sphinx_toolbox.collapse',
	'sphinx_a4doc',
	#'sphinx_design'
]

graphviz_output_format = "svg"

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

html_theme = 'furo'

html_theme_options = {
	"footer_icons": [{
		"name": "GitHub",
		"url": "https://github.com/wrapl/minilang",
		"html": """
			<svg stroke="currentColor" fill="currentColor" stroke-width="0" viewBox="0 0 16 16">
				<path fill-rule="evenodd" d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0 0 16 8c0-4.42-3.58-8-8-8z"/>
			</svg>
		""",
		"class": ""
	}]
}

github_url = "https://github.com/wrapl/minilang"

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
	sphinx.add_css_file('css/custom.css')
