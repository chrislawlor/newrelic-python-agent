import os
import time
import sys

import newrelic.core.metric

_samplers = []

def register_sampler(klass, args=()):
    """Register sampler to be associated with application objects.
    The class types and args to be used to constructor when created
    need to be supplied.

    """

    _samplers.append((klass, args))

def create_samplers():
    """Return a list of instances of all the registered samplers.

    """

    return [klass(*args) for klass, args in _samplers]

def _cpu_count():
    """Return the number of processors host hardware provides.

    """

    # TODO For more methods of determining this if required see
    # http://stackoverflow.com/questions/1006289.

    # Python 2.6+.

    try:
        import multiprocessing
        return multiprocessing.cpu_count()
    except (ImportError, NotImplementedError):
        pass

    # POSIX Systems.

    try:
        res = int(os.sysconf('SC_NPROCESSORS_ONLN'))
        if res > 0:
            return res
    except (AttributeError, ValueError):
        pass

    # Fallback to indicating only a single processor.

    return 1

class CPUSampler(object):

    # TODO Original comment in code provided by Saxon is "CPU
    # times are not sampled". Not sure what that is meant to me
    # so need to find out.

    def __init__(self):
        self._last_timestamp = time.time()
        self._times = os.times()

    def value_metrics(self):
        now = time.time()
        new_times = os.times()

        elapsed_time = now - self._last_timestamp

        user_time = new_times[0] - self._times[0]

        utilization = user_time / (elapsed_time*_cpu_count())

        self._last_timestamp = now
        self._times = new_times

        yield newrelic.core.metric.ValueMetric(
                name='CPU/User Time', value=user_time)
        yield newrelic.core.metric.ValueMetric(
                name='CPU/User/Utilization', value=utilization)

register_sampler(CPUSampler)

def _memory_used():
    """Returns the amount of resident memory in use in MBs.

    """

    # FIXME  Need to fill out appropriate methods here for
    # different platforms.

    # For Linux use the proc filesystem. Use 'statm' as easier
    # to parse than 'status' file.
    #
    #   /proc/[number]/statm
    #          Provides information about memory usage, measured in pages.
    #          The columns are:
    #
    #              size       total program size
    #                         (same as VmSize in /proc/[number]/status)
    #              resident   resident set size
    #                         (same as VmRSS in /proc/[number]/status)
    #              share      shared pages (from shared mappings)
    #              text       text (code)
    #              lib        library (unused in Linux 2.6)
    #              data       data + stack
    #              dt         dirty pages (unused in Linux 2.6)

    if sys.platform == 'linux2':
        pid = os.getpid()
        statm = '/proc/%d/statm' % pid
        fp = None

        try:
            fp = open(statm, 'r')
            return float(fp.read().split()[1]) / 1024
        except:
            pass
        finally:
            if fp:
                fp.close()

    # Fallback to trying to use getrusage(). The units returned
    # can differ based on platform. Assume 1024 byte blocks as
    # default.

    try:
        import resource
        rusage = resource.getrusage(resource.RUSAGE_SELF)
        if sys.platform == 'darwin':
            return float(rusage.ru_maxrss) / (1024*1024)
        else:
            return float(rusage.ru_maxrss) / 1024
    except:
        pass

    # Fallback to indicating no memory usage.

    return 0

class MemorySampler(object):

    def value_metrics(self):
        yield newrelic.core.metric.ValueMetric(
                name='Memory/Physical', value=_memory_used())

register_sampler(MemorySampler)
