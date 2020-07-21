#! /usr/bin/python3
import sys
import time
import signal

class TimeoutError(AssertionError):
    def __init__(self, value="Timed Out"):
        self.value = value

    def __str__(self):
        return repr(self.value)

def _raise_exception(exception, exception_message):
    if exception_message is None:
        raise exception()
    else:
        raise exception(exception_message)

def timeout(seconds=None, use_signals=True, timeout_exception=TimeoutError, exception_message=None):
    def decorate(function):
        if not seconds:
            return function

        def handler(signum, frame):
            _raise_exception(timeout_exception, exception_message)

        def new_function(*args, **kwargs):
            new_seconds = kwargs.pop('timeout', seconds)
            if new_seconds:
                old = signal.signal(signal.SIGALRM, handler)
                signal.setitimer(signal.ITIMER_REAL, new_seconds)
            try:
                return function(*args, **kwargs)
            finally:
                if new_seconds:
                    signal.setitimer(signal.ITIMER_REAL, 0)
                    signal.signal(signal.SIGALRM, old)
        return new_function
    return decorate