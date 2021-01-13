import numpy as np
import pytest
from packaging.utils import Version
import pnumpy

old_numpy = Version(np.__version__) < Version('1.18')


@pytest.fixture(scope='session')
def initialize_pnumpy():
    from numpy.core._multiarray_umath import __cpu_features__ as cpu
    if not cpu['AVX2']:
        pytest.skip('pnumpy.initialize requires AVX2')
    pnumpy.initialize()

@pytest.fixture(scope='function')
def rng():
    if old_numpy:
        class OldRNG(np.random.RandomState):
            pass
        rng = OldRNG(1234)
        rng.random = rng.random_sample
        rng.integers = rng.randint
        return rng
    else:
        return np.random.default_rng(1234)
