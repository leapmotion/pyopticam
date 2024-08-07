import sys, re, os

try:
    from skbuild import setup
    import nanobind
except ImportError:
    print("The preferred way to invoke 'setup.py' is via pip, as in 'pip "
          "install .'. If you wish to run the setup script directly, you must "
          "first install the build dependencies listed in pyproject.toml!",
          file=sys.stderr)
    raise

setup(
    name="pyopticam",
    version="0.0.2",
    author="Johnathon Selstad",
    author_email="john.selstad@ultraleap.com",
    description="A nanobind wrapper for NaturalPoint's Optitrack Camera SDK",
    url="https://github.com/leapmotion/pyopticam",
    license="BSD",
    packages=['pyopticam'],
    package_dir={'': 'src'},
    cmake_install_dir="src/pyopticam",
    include_package_data=True,
    python_requires=">=3.8"
)
