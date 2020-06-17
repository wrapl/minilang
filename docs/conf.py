from docutils.parsers.rst import Directive, roles
from docutils import nodes
import re
from pygments import lexers
import inspect
import sphinx_bootstrap_theme

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

extensions = []

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
#html_theme = "sphinx_rtd_theme"
html_theme = 'bootstrap'
html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

html_theme_options = {
    'bootswatch_theme': "united"
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

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
   :langauge: c
   :class: highlight
"""

def setup(sphinx):
	import sys, os
	sys.path.insert(0, os.path.abspath('./_util'))
	from minilang import MinilangLexer, minilangDomain
	sphinx.add_lexer("mini", MinilangLexer())
	lexers.LEXERS['mini'] = ('minilang', 'Minilang', ('mini',), ('*.mini', '*.rabs'), ('text/x-mini',))
	#sphinx.add_domain(minilangDomain) 
	sphinx.add_directive('folders', FoldersDirective)
	sphinx.add_stylesheet('css/custom.css')
