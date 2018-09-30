import unittest
from subprocess import Popen, PIPE, TimeoutExpired

# default timeout in seconds
DEFAULT_TIMEOUT = 10

# default list of parameters when comparing script output
DEFAULT_PARAMS = ["status", "stdout", "stderr"]

# location of the flassh executable
FLASSH_PATH = "../build/flassh"

# run a script, returns {status, stdout, stderr}
def runScript(args, stdin = None, timeout = None):
    if timeout is None:
        timeout = DEFAULT_TIMEOUT
    p = Popen(args, stdout=PIPE, stderr=PIPE, stdin=PIPE)
    try:
        output = p.communicate(stdin, timeout)
    except TimeoutExpired as e:
        p.kill()
        raise e
    return {
        "status": p.returncode,
        "stdout": output[0],
        "stderr": output[1],
    }

class FlasshTestCase(unittest.TestCase):
    # missing values in `params` are treated as "don't care"
    def assertOutputEqual(self, output, compare, params = DEFAULT_PARAMS):
        if "status" in params:
            self.assertEqual(output["status"], compare["status"])
        if "stdout" in params:
            self.assertEqual(output["stdout"], compare["stdout"])
        if "stderr" in params:
            self.assertEqual(output["stderr"], compare["stderr"])

    # assert both cmds have same behavior
    def assertCmdsEqual(self, cmd1, cmd2, **kwargs):
        stdin = kwargs["stdin"] if "stdin" in kwargs else None
        timeout = kwargs["timeout"] if "timeout" in kwargs else None
        params = kwargs["params"] if "params" in kwargs else DEFAULT_PARAMS
        out1 = runScript(cmd1, stdin, timeout)
        out2 = runScript(cmd2, stdin, timeout)
        self.assertOutputEqual(out1, out2, params)
