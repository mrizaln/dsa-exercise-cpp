#!/usr/bin/env python3

from argparse import ArgumentParser
import subprocess


def main():
    parser = ArgumentParser(description="Test script")

    parser.add_argument("target", nargs="?", default="all")
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--release", action="store_true")

    args = parser.parse_args()
    print(f"args: {vars(args)}\n")

    build_type = "release" if args.release else "debug"
    preset = f"conan-{build_type}"
    target = args.target

    if not args.no_build:
        cmake_cmd = ["cmake", "--build", "--preset", preset, "--target", target]
        subprocess.run(cmake_cmd).check_returncode()
        print()

    target_re = ".*" if target == "all" else target
    ctest_cmd = ["ctest", "--preset", preset, "--output-on-failure", "-R", target_re]

    subprocess.run(ctest_cmd).check_returncode()


if __name__ == "__main__":
    ret = main()
    exit(ret)
