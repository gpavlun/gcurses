#!/bin/sh
set -e

cargo build
cp target/debug/test bin/test