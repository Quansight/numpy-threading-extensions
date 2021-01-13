"""
Call ``initialize`` to setup the package. This imports and scans NumPy,
replacing all the inner loops of UFuncs with wrapped versions. Then you can
enable/disable any of the subsystems:

  - threading

    Threading will kick in when the number of elements to be processed is more
    than 50,000. It will break the operation into chunks. Each chunk will be
    executed in its own thread.
  - ledger

    The ledger (disabled in version 0.1) records data on each loop execution to
    enable more accurate heuristics on memory allocation, threading behavior
    and reporting for logging and benchmarking.
  - recycler

    Once we can change the NumPy memory allocation strategy, we can use the
    data from the ledger to create more performant memory caches.
  - atop

    NumPy is making progress with faster inner loops, but an outside package
    can iterate faster to provide even faster ones. Since the Universal SIMD
    loops are in a state of flux at this time, this is disabled for version
    0.1.


"""
__version__ = '0.0.0'
__all__ = [
    'initialize', 'atop_enable', 'atop_disable', 'atop_isenabled', 'atop_info', 'atop_setworkers','cpustring',
    'thread_enable', 'thread_disable', 'thread_isenabled', 'thread_getworkers', 'thread_setworkers', 'thread_zigzag',
    'ledger_enable', 'ledger_disable', 'ledger_isenabled', 'ledger_info',
    'recycler_enable', 'recycler_disable', 'recycler_isenabled', 'recycler_info',
    'timer_gettsc','timer_getutc']
import pnumpy._pnumpy as _pnumpy
from pnumpy._pnumpy import atop_enable, atop_disable, atop_isenabled, atop_info, atop_setworkers, cpustring 
from pnumpy._pnumpy import thread_enable, thread_disable, thread_isenabled, thread_getworkers, thread_setworkers, thread_zigzag
from pnumpy._pnumpy import timer_gettsc, timer_getutc
from pnumpy._pnumpy import ledger_enable, ledger_disable, ledger_isenabled, ledger_info
from pnumpy._pnumpy import recycler_enable, recycler_disable, recycler_isenabled, recycler_info
from pnumpy._pnumpy import getitem, lexsort32, lexsort64

from .sort import sort, lexsort, argsort, argmin, argmax, searchsorted
from .benchmark import benchmark, benchmark_func
from .recarray import recarray_to_colmajor
import numpy as np

def init():
    """
    Called at load time to start the atop and threading engines.
    
    Parameters
    ----------
    None

    See Also
    --------

    """
    
    import platform
    if platform.system() == 'Linux':
        from .cpu import cpu_count_linux
        logical,physical = cpu_count_linux()
        _pnumpy.initialize()
    else:
        _pnumpy.initialize()

def initialize():
    """
    To be deprecated.  Call init() instead.

    Parameters
    ----------
    None
    """
    init()

def enable():
    """
    Call to enable the atop engine, use threads, and hook numpy functions.

    Parameters
    ----------
    None

    Returns
    -------
    None

    See Also
    --------
    pn.disable
    pn.atop_info
    """
    atop_enable()
    thread_enable()

def disable():
    """
    Call to disable the atop engine, stop any threads, and unhook numpy functions.

    Parameters
    ----------
    None

    Returns
    -------
    None

    See Also
    --------
    pn.enable
    pn.atop_info
    """
    atop_disable()
    thread_disable()


# start the engine by default
# TODO: check environment variable
init()

