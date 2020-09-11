import sys
import os
import pkgutil
import importlib
__all__ = list(module for _, module, _ in pkgutil.iter_modules(sys.path))

for module in __all__:
    try:
        module_import = importlib.import_module(module)
        try:
            version = module_import.__version__
        except:
            version = None
        print(module, "ok", version)
    except:
        print(module, "failed")
