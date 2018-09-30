#!/usr/bin/env python3
# technically these aren't unit tests, but whatever
import unittest
from util import FlasshTestCase, DEFAULT_PARAMS, FLASSH_PATH

# test bash compatibility by running the same script in flassh and bash
class TestBashCompat(FlasshTestCase):
    def assertBashCompat(self, script, compareStderr = True):
        params = DEFAULT_PARAMS if compareStderr else ["status", "stdout"]
        self.assertCmdsEqual(["bash", script], [FLASSH_PATH, script], params=params)

    def test_all(self):
        self.assertBashCompat("bash_compat/all.sh")

    def test_nonexistant(self):
        self.assertBashCompat("🤔.sh", False)

    def test_basic(self):
        self.assertBashCompat("bash_compat/basic.sh")

    def test_quotes_escapes(self):
        self.assertBashCompat("bash_compat/quotes_escapes.sh")
        self.assertBashCompat("bash_compat/fail_quote.sh", False)
        self.assertBashCompat("bash_compat/fail_escape.sh", False)

    def test_whitespace(self):
        self.assertBashCompat("bash_compat/whitespace.sh")
    
    def test_pipe(self):
        self.assertBashCompat("bash_compat/pipe.sh")

    # TODO: test I/O redirection, subshell, background processes, etc


if __name__ == "__main__":
    unittest.main()
