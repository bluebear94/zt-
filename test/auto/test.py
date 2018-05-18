#!/usr/bin/env python3

import difflib
from pathlib import Path
import subprocess
import sys

def stripLines(s):
  return '\n'.join([l.strip() for l in s.splitlines()])

casesDir = Path("test/auto/cases/")
outputDir = Path("test/auto/output/")

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
  p = subprocess.run(["build/sca_e_kozet", str(ztPath), str(inp)],
    stdout=subprocess.PIPE, encoding="utf8")
  actualStr = p.stdout
  with output.open("w") as fh:
    fh.write(actualStr)
  with expout.open("r") as fh:
    expectedStr = fh.read(None)
    expectedStr = stripLines(expectedStr)
    actualStr = stripLines(actualStr)
    if expectedStr != actualStr:
      # fail
      print("Test {} failed".format(caseName), file=sys.stderr)
      nFail += 1
      diff = difflib.unified_diff(
        expectedStr, actualStr,
        fromfile=str(expout), tofile=str(output))
      print(diff, file=sys.stderr)
      with diffpath.open("w") as fh:
        fh.write(diff)
    else:
      # pass
      print("Test {} passed".format(caseName), file=sys.stderr)
      nPass += 1

print("{} passed, {} failed".format(nPass, nFail), file=sys.stderr)

if nFail != 0: sys.exit(1)
