#!/usr/bin/env python3

import difflib
from pathlib import Path
import subprocess
import sys

def stripLines(s):
  return [l.strip() + "\n" for l in s.splitlines()]

execPath = sys.argv[1]
testDir = Path(sys.argv[2])
casesDir = testDir / "auto/cases"
outputDir = testDir / "auto/output"

outputDir.mkdir(parents=True, exist_ok=True)

allCases = sorted(casesDir.glob("*.zt"))

nPass = 0
nFail = 0

for ztPath in allCases:
  caseName = ztPath.stem
  inp = casesDir / ("words-" + caseName + ".txt")
  expout = casesDir / ("expected-" + caseName + ".txt")
  output = outputDir / ("actual-" + caseName + ".txt")
  diffpath = outputDir / (caseName + ".diff")
  p = subprocess.run([execPath, str(ztPath), str(inp)],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding="utf8")
  actualStr = p.stdout
  with output.open("w") as fh:
    fh.write(actualStr)
  with expout.open("r") as fh:
    expectedStr = fh.read(None)
    expectedLines = stripLines(expectedStr)
    actualLines = stripLines(actualStr)
    if expectedLines != actualLines:
      # fail
      print("Test {} failed".format(caseName), file=sys.stderr)
      nFail += 1
      diff = difflib.unified_diff(
        expectedLines, actualLines,
        fromfile=str(expout), tofile=str(output))
      sdiff = ''.join(diff)
      sys.stderr.write(sdiff)
      with diffpath.open("w") as fh:
        fh.write(sdiff)
    else:
      # pass
      print("Test {} passed".format(caseName), file=sys.stderr)
      nPass += 1

print("{} passed, {} failed".format(nPass, nFail), file=sys.stderr)

if nFail != 0: sys.exit(1)
