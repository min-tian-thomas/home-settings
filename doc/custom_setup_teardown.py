import unittest
import functools
from parameterized import parameterized


def custom_setup_teardown(setup_func, teardown_func):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            try:
                setup_func(self, *args, **kwargs)
                func(self, *args, **kwargs)
            finally:
                teardown_func(self, *args, **kwargs)

        return wrapper

    return decorator


class MyTest(unittest.TestCase):
    def setUp(self):
        print("MyTest::setUp")

    def tearDown(self):
        print("MyTest::tearDown")

    def inner_setup(self, username, age):
        print(f"inner_scope setUp, {username}, {age}")

    def inner_teardown(self, username, age):
        print(f"inner_scope tearDown, {username}, {age}")

    @parameterized.expand(
        [
            ("thomas", 12),
            ("james", 13),
        ]
    )
    @custom_setup_teardown(inner_setup, inner_teardown)
    def test_simple(self, username, age):
        print(f"{self._testMethodName}, {username}, {age}")
