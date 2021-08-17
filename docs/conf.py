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
# http://www.sphinx-doc.org/en/master/config

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

extensions = ['sphinx.ext.graphviz']


# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
#html_theme = "sphinx_material"
#html_theme_options = {
#	'color_primary': 'orange',
#	'color_accent': 'yellow',
#	'globaltoc_depth': 3
#}
#
#html_sidebars = {
#    "**": ["logo-text.html", "globaltoc.html", "localtoc.html", "searchbox.html"]
#}
html_theme = "pydata_sphinx_theme"
html_theme_options = {
	"collapse_navigation": True,
	"page_sidebar_items": []
}
html_sidebars = {
    "**": ["search-field", "page-toc", "sidebar-nav-bs"]
}

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
