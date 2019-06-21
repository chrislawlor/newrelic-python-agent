import os.path
import tempfile

import newrelic.common.log_file
import newrelic.config

from newrelic.admin.generate_config import generate_config

_newrelic_ini = os.path.join(tempfile.gettempdir(), 'newrelic.ini')
generate_config(('BOGUS LICENSE KEY HERE', _newrelic_ini))


class TimeInitialize(object):

    params = (False, True)
    param_names = ('config_file', )

    # NOTE: A good portion of the time taken up during initialize is in
    # importing pkg_resources. Unfortunately however it is quite difficult to
    # benchmark this. First we would need to run benchmarks both with and
    # without pkg_resources installed. Second, because python caches modules,
    # we would need to disable cache lookup. In lieu of doing this, we have
    # decided to only test with pkg_resources and let the caching happen with
    # the knowledge that if we could avoid the import of pkg_resources
    # altogether we would get a pretty big overhead reduction win.

    def time_initialize(self, use_config_file):
        newrelic.common.log_file._initialized = False
        newrelic.config._configuration_done = False
        newrelic.config._instrumentation_done = False
        newrelic.config._data_sources_done = False
        config_file = use_config_file and _newrelic_ini
        newrelic.config.initialize(config_file=config_file, log_file='stdout')