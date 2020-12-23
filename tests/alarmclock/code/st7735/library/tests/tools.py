import sys


def force_reimport(module):
    """Force the module under test to be re-imported.

    Because pytest runs all tests within the same scope (this makes me cry)
    we have to do some manual housekeeping to avoid tests polluting each other.

    Since conftest.py already does some sys.modules mangling I see no reason not to
    do the same thing here.
    """
    if "." in module:
        steps = module.split(".")
    else:
        steps = [module]

    for i in range(len(steps)):
        module = ".".join(steps[0:i + 1])
        try:
            del sys.modules[module]
        except KeyError:
            pass
