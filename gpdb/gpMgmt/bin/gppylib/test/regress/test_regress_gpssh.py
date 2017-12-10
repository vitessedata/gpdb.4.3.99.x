#!/usr/bin/env python

import os, signal, time, re
import unittest2 as unittest

class GpsshTestCase(unittest.TestCase):

    # return count of stranded ssh processes 
    def searchForProcessOrChildren(self):

        def runPs():
            p = subprocess.Popen("dg ps -p %s" % pid,
                                 shell=True,
                                 stdout = subprocess.PIPE,
                                 stderr = subprocess.PIPE)
            out, err = p.communicate()
            rc = p.wait()
            if rc:
                sys.exit('Cannot run ps: ' + err)

            proc = []
            for line in out.split('\n'):
                (pid, ppid, uid, euid, gid, egid, cmd) = line.split(' ', 7)
                proc += [(int(pid), int(ppid), int(uid), int(euid), cmd)]

            return proc
        
        euid = os.getuid()
        count = 0

        for (p_pid, p_ppid, p_uid, p_euid, cmd) in runPs():
            if p_euid != euid:
                continue
            if not 'ssh' in cmd:
                continue
            if p_ppid != 1:
                continue

            count += 1

        return count


    def test00_gpssh_sighup(self):
        """Verify that gppsh handles sighup 
           and terminates cleanly.
        """

        before_count = self.searchForProcessOrChildren()

        p = subprocess.Popen("gpssh -h localhost", shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        pid = p.pid

        time.sleep(3)

        try:
            os.kill(int(pid), signal.SIGHUP)
        except Exception:
            pass

        max_attempts = 6
        for i in range(max_attempts):
            after_count = self.searchForProcessOrChildren()
            error_count = after_count - before_count
            if error_count:
                if (i + 1) == max_attempts:
                    self.fail("Found %d new stranded gpssh processes after issuing sig HUP" % error_count)
                time.sleep(.5)
 
if __name__ == "__main__":
    unittest.main()
