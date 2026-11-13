# -*- coding: utf-8 -*-
#
# MeshKit Python documentation build configuration file.
# Follows openswmm.engine's python/docs/conf.py.

import importlib.abc
import importlib.util
import os
import sys

# ============================================================================
# .pyi stub-file import hook
# ============================================================================
# Sphinx autodoc works by *importing* modules and inspecting the live objects.
# The native extension meshkit._meshkit is a compiled module that a docs build
# does not have. Its stub, python/meshkit/_meshkit.pyi, holds the signatures and
# docstrings as valid Python (docstring-only bodies), so this meta-path finder
# loads the stub in place of the extension. It is installed first, so the stub
# wins even when a compiled extension sits next to it.
# ============================================================================

_PACKAGE_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))


class _PyiStubFinder(importlib.abc.MetaPathFinder):
    """Meta-path finder that loads a native module's .pyi stub as the module."""

    _STUBS = ('meshkit._meshkit',)

    def find_spec(self, fullname, path, target=None):
        if fullname not in self._STUBS:
            return None
        pyi_path = os.path.join(_PACKAGE_ROOT, *fullname.split('.')) + '.pyi'
        if os.path.isfile(pyi_path):
            return importlib.util.spec_from_file_location(
                fullname, pyi_path, loader=_PyiStubLoader(pyi_path))
        return None


class _PyiStubLoader(importlib.abc.Loader):
    """Loader that executes a .pyi file as a regular Python module."""

    def __init__(self, pyi_path):
        self.pyi_path = pyi_path

    def create_module(self, spec):
        return None  # use default module creation

    def exec_module(self, module):
        module.__file__ = self.pyi_path
        with open(self.pyi_path, 'r', encoding='utf-8') as f:
            code = compile(f.read(), self.pyi_path, 'exec')
        exec(code, module.__dict__)


# Install the hook before everything else
sys.meta_path.insert(0, _PyiStubFinder())

# Add python/ to sys.path so autodoc finds the meshkit package
sys.path.insert(0, _PACKAGE_ROOT)

import meshkit  # noqa: E402

version = meshkit.__version__
release = meshkit.__version__

# -- General configuration ------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
    # sphinx-design: grid / grid-item-card directives on the landing page.
    'sphinx_design',
]

master_doc = 'index'

# -- Project information --------------------------------------------------

project = 'MeshKit'
copyright = '2026 Caleb Buahin'
author = 'Caleb Buahin'

# -- Napoleon settings (NumPy docstrings) ---------------------------------

napoleon_google_docstring = False
napoleon_numpy_docstring = True
napoleon_use_param = True
napoleon_use_rtype = True

# -- Autodoc settings -----------------------------------------------------
# The .pyi import hook above means autodoc imports the stub directly and reads
# its docstrings and type annotations. No mocking is needed.

autodoc_default_options = {
    'members': True,
    'member-order': 'bysource',
}
autodoc_typehints = 'description'
autodoc_typehints_format = 'short'

add_function_parentheses = True
add_module_names = False

# -- Intersphinx ----------------------------------------------------------

# Prefer current upstream inventories, but keep cross-references working when
# either documentation host is unavailable.  See _intersphinx/README.md for
# snapshot provenance and refresh instructions.  Other warnings remain fatal
# under the CI build's -W flag.
intersphinx_mapping = {
    'python': ('https://docs.python.org/3', (None, '_intersphinx/python.inv')),
    'numpy': ('https://numpy.org/doc/stable/', (None, '_intersphinx/numpy.inv')),
}
intersphinx_timeout = 10

# -- Options for HTML output ----------------------------------------------

language = 'en'
pygments_style = 'sphinx'

exclude_patterns = ['_build', '_intersphinx']

html_theme = 'pydata_sphinx_theme'

html_theme_options = {
    "logo": {
        "text": "MeshKit",
        "image_light": "images/hydrocouple_logo.png",
        "image_dark": "images/hydrocouple_logo.png",
    },
    # Top-nav cross-link back to the Doxygen C++ docs.
    # Deployment layout (see .github/workflows/documentation.yml):
    #     /         Doxygen docs (root)
    #     /python/  these (Sphinx) docs
    # ``../index.html`` resolves to the Doxygen root when both sites are
    # co-deployed under GitHub Pages.  When viewing the Sphinx docs
    # locally without the Doxygen bundle, this link 404s — that's an
    # accepted local-only limitation.
    "external_links": [
        {"name": "C++ API Docs", "url": "../index.html"},
    ],
    "icon_links": [
        {
            "name": "GitHub",
            "url": "https://github.com/HydroCouple/Meshkit",
            "icon": "fa-brands fa-github",
            "type": "fontawesome",
        },
        {
            "name": "PyPI",
            "url": "https://pypi.org/project/hydrocouple.meshkit",
            "icon": "fa-brands fa-python",
            "type": "fontawesome",
        },
    ],
    "use_edit_page_button": False,
    "show_toc_level": 2,
    "navbar_end": ["theme-switcher.html", "navbar-icon-links.html"],
}

html_logo = 'images/hydrocouple_logo.png'
html_title = 'MeshKit'
html_favicon = 'images/hydrocouple_logo.png'

html_static_path = ['_static']
html_css_files = ['meshkit.css']

htmlhelp_basename = 'meshkitdoc'
