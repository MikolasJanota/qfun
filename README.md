# qfun

A circuit-based QBF solver. The underlying algorithm is explained in [1].

# Building
Ideally, this should work out-of-the-box:

     ./configure [OPTIONS] && cd build && make

See `./configure -h` for configurations.

The configure script is responsible for downloading and compiling the selected SAT solver.

Remark on the cmake required version: it's at 3.24 at this point because it
lets me force static for zlib. If you don't care about static compilation,
lower versions of cmake should also work.

#  REFERENCES

1. Towards Generalization in QBF Solving via Machine Learning, Mikoláš Janota in SAT '18
