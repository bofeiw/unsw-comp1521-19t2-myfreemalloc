#!/usr/bin/env bash


make


diff <(./test1 5000) <(echo "+00000 (F, 5000)")  --ignore-all-space
diff <(./test1 10000) <(echo "+00000 (F,10000)") --ignore-all-space
diff <(./test1 100) <(echo "+00000 (F, 4096)") --ignore-all-space
diff <(./test1 5001) <(echo "+00000 (F, 5004)") --ignore-all-space