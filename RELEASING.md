# Releasing simple-metricd

## Version source of truth

`CMakeLists.txt` and the project `VERSION` file set the product version. Keep **CHANGELOG.md**, packaging metadata, and git tags aligned with that version for the release you publish.

## Pre-release

1. Update [CHANGELOG.md](CHANGELOG.md) (`Unreleased` → new section with date).
2. Bump version in CMake / `VERSION` / `include/simple-metricd/version.hpp` if needed.
3. Run the [project/RELEASE_CHECKLIST.md](project/RELEASE_CHECKLIST.md) when present.
4. Prefer `project/PROGRESS_REPORT.md` over roadmap checkmarks when deciding readiness.

## Build and test

```sh
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/simple-metricd --version
./build/simple-metricd --test-config --config config/templates/development.conf
./build/metricctl --config config/templates/development.conf list
```

## Tag and publish

```sh
git tag -a v0.1.0 -m "Release v0.1.0"
git push origin v0.1.0
```

1. Create a GitHub Release for the tag; paste the matching **CHANGELOG.md** section.
2. Optional: attach CPack / `make package` artifacts with `gh release upload`.
