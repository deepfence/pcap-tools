#! /usr/bin/env bash

ZEEK_BENCHMARK_ENDPOINT="/zeek"

# Setting this causes any command failures to immediately cause the script to fail.
set -e

# Don't do this unless the user has access to the encrypted variables. This will
# basically exclude any PR that doesn't come from the main zeek repo.
if [ "${CIRRUS_USER_PERMISSION}" != "admin" -a "${CIRRUS_USER_PERMISSION}" != "write" ]; then
    echo "Benchmarks are skipped for repositories outside of the main Zeek project"
    exit 0
fi

BUILD_URL="https://api.cirrus-ci.com/v1/artifact/build/${CIRRUS_BUILD_ID}/${CIRRUS_TASK_NAME}/upload_binary/build.tgz"

# Generate an md5 hash of the build file. We can do this here because the path to the
# file still exists from the prior scripts.
BUILD_HASH=$(sha256sum build.tgz | awk '{print $1}')

# Generate an HMAC digest for the path plus a timestamp to send as an authentication
# header. Openssl outputs a hex string here so there's no need to base64 encode it.
TIMESTAMP=$(date -u +'%s')
HMAC_DIGEST=$(echo "${ZEEK_BENCHMARK_ENDPOINT}-${TIMESTAMP}-${BUILD_HASH}" | openssl dgst -sha256 -hmac ${ZEEK_BENCHMARK_HMAC_KEY} | awk '{print $2}')

TARGET="https://${ZEEK_BENCHMARK_HOST}:${ZEEK_BENCHMARK_PORT}${ZEEK_BENCHMARK_ENDPOINT}"

# Turn this back off because we want to be able to capture the output from curl if
# it fails.
set +e

# Make a request to the benchmark host.
RESULTS=$(curl -sS --stderr - --fail --insecure -X POST -H "Zeek-HMAC: ${HMAC_DIGEST}" -H "Zeek-HMAC-Timestamp: ${TIMESTAMP}" "${TARGET}?branch=${CIRRUS_BRANCH}&build=${BUILD_URL}&build_hash=${BUILD_HASH}")
STATUS=$?

# If we got a bad status back from the host, we want to make sure to mask the host
# and port from the output.
if [ $STATUS -ne 0 ]; then
    RESULTS=$(echo "${RESULTS}" | sed "s/${ZEEK_BENCHMARK_HOST}/<secret>/g" | sed "s/:${ZEEK_BENCHMARK_PORT}/:<secret>/g")
fi

echo "$RESULTS"
exit $STATUS
