#!/usr/bin/env python3
import sys, os

path = sys.argv[1]
limit = int(sys.argv[2])
size = os.path.getsize(path)
if size > limit:
    print(f"kernel too big: {size} bytes (max {limit})", file=sys.stderr)
    sys.exit(1)
print(f"kernel binary: {size} bytes OK")
