#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-8080}"
HOST="${HOST:-127.0.0.1}"
BASE_URL="http://${HOST}:${PORT}"
BUILD_DIR="${BUILD_DIR:-build}"
BIN="${BUILD_DIR}/fman"
SERVER_LOG="${BUILD_DIR}/fman-http.log"
SERVER_PY="${BUILD_DIR}/fman_test_server.py"

PARSER_NEGATIVE_LOG="${BUILD_DIR}/fman-parser-negative.out"

GENERATED_FILES=(
    "${BUILD_DIR}/test.bin"
    "${BUILD_DIR}/smoke.fman"
    "${BUILD_DIR}/fail.fman"
    "${BUILD_DIR}/parser_bad_missing_url.fman"
    "${BUILD_DIR}/parser_bad_command.fman"
    "${BUILD_DIR}/parser_bad_assertion.fman"
    "${BUILD_DIR}/parallel.fman"
    "${BUILD_DIR}/fman_test_server.py"
    "${BUILD_DIR}/fman-http.log"
    "${BUILD_DIR}/fman-parser-negative.out"
    "runner.db"
    "test_runtime.db"
    ".fman/default.db"
)

cleanup() {
    if [ -n "${SERVER_PID:-}" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi

    for file in "${GENERATED_FILES[@]}"; do
        rm -f "$file" 2>/dev/null || true
    done

    rmdir ".fman" 2>/dev/null || true
}

trap cleanup EXIT


echo "[1/10] Configure"
cmake -S . -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "[2/10] Build"
cmake --build "$BUILD_DIR"

echo "[3/10] Verify binary and README assets"

if [ ! -x "$BIN" ]; then
    echo "Expected executable not found: $BIN"
    exit 1
fi

for asset in \
    images/demo.gif \
    images/arch.png \
    images/performance.png \
    images/throughput.png \
    images/memory.png
do
    if [ ! -f "$asset" ]; then
        echo "Missing README asset: $asset"
        exit 1
    fi
done

echo "[4/10] Prepare test files"

printf '\x01\x02\x7F\x8A\xFF' > "${BUILD_DIR}/test.bin"

cat > "${BUILD_DIR}/smoke.fman" << EOF
GET ${BASE_URL}/ok
EXPECT status 200
EXPECT body contains "OK"

GET ${BASE_URL}/header
EXPECT status 200
EXPECT header X-Test fman
EOF

cat > "${BUILD_DIR}/fail.fman" << EOF
GET ${BASE_URL}/ok
EXPECT status 404
EOF

cat > "${BUILD_DIR}/parser_bad_missing_url.fman" << EOF
GET
EXPECT status 200
EOF

cat > "${BUILD_DIR}/parser_bad_command.fman" << EOF
POSTT ${BASE_URL}/ok
EXPECT status 200
EOF

cat > "${BUILD_DIR}/parser_bad_assertion.fman" << EOF
GET ${BASE_URL}/ok
EXPECT banana 123
EOF

cat > "${BUILD_DIR}/parallel.fman" << EOF
GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200

GET ${BASE_URL}/slow/200
EXPECT status 200
EOF

cat > "$SERVER_PY" << 'PY'
#!/usr/bin/env python3
import os
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = os.environ.get("HOST", "127.0.0.1")
PORT = int(os.environ.get("PORT", "8080"))

class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        return

    def send_text(self, status, body, headers=None):
        data = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(data)))

        if headers:
            for key, value in headers.items():
                self.send_header(key, value)

        self.end_headers()
        self.wfile.write(data)

    def send_binary(self, status, data):
        self.send_response(status)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = self.path.split("?", 1)[0]

        if path == "/ok":
            self.send_text(200, "OK")
            return

        if path == "/text":
            self.send_text(200, "Example Domain")
            return

        if path == "/header":
            self.send_text(200, "OK", {"X-Test": "fman"})
            return

        if path == "/fail":
            self.send_text(500, "FAIL")
            return

        if path == "/test.bin" or path == "/bin":
            self.send_binary(200, bytes([0x01, 0x02, 0x7F, 0x8A, 0xFF]))
            return

        if path.startswith("/slow/"):
            try:
                delay_ms = int(path.rsplit("/", 1)[1])
            except ValueError:
                self.send_text(400, "bad delay")
                return

            time.sleep(delay_ms / 1000.0)
            self.send_text(200, "SLOW OK")
            return

        self.send_text(404, "NOT FOUND")

server = ThreadingHTTPServer((HOST, PORT), Handler)
server.serve_forever()
PY

chmod +x "$SERVER_PY"

echo "[5/10] Start HTTP test server"
rm -f "$SERVER_LOG"

HOST="$HOST" PORT="$PORT" python3 "$SERVER_PY" >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

echo "Waiting for server at ${BASE_URL}/ok"

SERVER_READY=0
for _ in {1..50}; do
    if curl -fsS "${BASE_URL}/ok" >/dev/null 2>&1; then
        SERVER_READY=1
        break
    fi

    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "HTTP server exited early."
        cat "$SERVER_LOG" || true
        exit 1
    fi

    sleep 0.2
done

if [ "$SERVER_READY" -ne 1 ]; then
    echo "HTTP test server did not become ready."
    cat "$SERVER_LOG" || true
    exit 1
fi

echo "[6/10] Unit/integration tests"
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "[7/10] CLI smoke tests"

"$BIN" get "${BASE_URL}/text" \
    --expect-status 200 \
    --expect-body "Example Domain"

set +e
"$BIN" get "${BASE_URL}/text" \
    --expect-status 404 \
    --expect-body "Example Domain"
FAIL_CODE=$?
set -e

if [ "$FAIL_CODE" -eq 0 ]; then
    echo "Expected failing CLI assertion test to return non-zero, got 0."
    exit 1
fi

"$BIN" get "${BASE_URL}/test.bin" \
    --expect-status 200 \
    --expect-body hex:7F8A

echo "[8/10] DSL smoke and failure tests"

"$BIN" run "${BUILD_DIR}/smoke.fman"

set +e
"$BIN" run "${BUILD_DIR}/fail.fman"
FAIL_CODE=$?
set -e

if [ "$FAIL_CODE" -eq 0 ]; then
    echo "Expected failing DSL assertion test to return non-zero, got 0."
    exit 1
fi

for bad_file in \
    "${BUILD_DIR}/parser_bad_missing_url.fman" \
    "${BUILD_DIR}/parser_bad_command.fman" \
    "${BUILD_DIR}/parser_bad_assertion.fman"
do
    set +e
    "$BIN" run "$bad_file" >"$PARSER_NEGATIVE_LOG" 2>&1
    FAIL_CODE=$?
    set -e

    if [ "$FAIL_CODE" -eq 0 ]; then
        echo "Expected parser failure for $bad_file, got success."
        cat "$PARSER_NEGATIVE_LOG" || true
        exit 1
    fi
done

echo "[9/10] Parallel RunPlan test"

python3 - "$BIN" "${BUILD_DIR}/parallel.fman" << 'PY'
import subprocess
import sys
import time

binary = sys.argv[1]
plan = sys.argv[2]

start = time.perf_counter()
result = subprocess.run([binary, "run", plan])
elapsed = time.perf_counter() - start

if result.returncode != 0:
    print(f"Parallel DSL run failed with exit code {result.returncode}")
    sys.exit(result.returncode)

print(f"Parallel DSL run elapsed: {elapsed:.3f} s")

# 8 requests * 200 ms = ~1.6 s if effectively sequential.
# Keep the threshold loose enough for slow machines but low enough to catch sequential execution.
if elapsed > 1.30:
    print("Parallel runtime check failed: elapsed time suggests sequential execution.")
    sys.exit(1)
PY

echo "[10/10] Valgrind"

if command -v valgrind >/dev/null 2>&1; then
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --error-exitcode=99 \
        --suppressions=tools/valgrind.supp \
        "$BIN" get "${BASE_URL}/test.bin" \
            --expect-status 200 \
            --expect-body hex:7F8A

    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --error-exitcode=99 \
        --suppressions=tools/valgrind.supp \
        "$BIN" run "${BUILD_DIR}/smoke.fman"
else
    echo "valgrind not found, skipping."
fi

echo
echo "All checks passed."