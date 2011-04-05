import unittest
import time
import sys

import _newrelic

settings = _newrelic.settings()
settings.logfile = "%s.log" % __file__
settings.loglevel = _newrelic.LOG_VERBOSEDEBUG

application = _newrelic.application("UnitTests")

#@_newrelic.external_trace(argnum=0)
def _test_function_1(url):
    time.sleep(1.0)
_test_function_1 = _newrelic.external_trace(argnum=0)(_test_function_1)

class ExternalTraceTests(unittest.TestCase):

    def setUp(self):
        _newrelic.log(_newrelic.LOG_DEBUG, "STARTING - %s" %
                      self._testMethodName)

    def tearDown(self):
        _newrelic.log(_newrelic.LOG_DEBUG, "STOPPING - %s" %
                      self._testMethodName)

    def test_external_trace(self):
        environ = { "REQUEST_URI": "/external_trace" }
        transaction = _newrelic.WebTransaction(application, environ)
        with transaction:
            time.sleep(0.1)
            with _newrelic.ExternalTrace(transaction, "http://localhost/"):
                time.sleep(0.1)
            time.sleep(0.1)

    def test_transaction_not_running(self):
        environ = { "REQUEST_URI": "/transaction_not_running" }
        transaction = _newrelic.WebTransaction(application, environ)
        try:
            with _newrelic.ExternalTrace(transaction, "http://localhost/"):
                time.sleep(0.1)
        except RuntimeError:
            pass

    def test_external_trace_decorator(self):
        environ = { "REQUEST_URI": "/external_trace_decorator" }
        transaction = _newrelic.WebTransaction(application, environ)
        with transaction:
            time.sleep(0.1)
            _test_function_1("http://localhost/")
            time.sleep(0.1)

    def test_external_trace_decorator_error(self):
        environ = { "REQUEST_URI": "/external_trace_decorator_error" }
        transaction = _newrelic.WebTransaction(application, environ)
        with transaction:
            try:
                _test_function_1("http://localhost/", None)
            except TypeError:
                pass

if __name__ == '__main__':
    unittest.main()