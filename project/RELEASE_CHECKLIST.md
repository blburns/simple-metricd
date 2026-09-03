# Release checklist — simple-metricd

Use before cutting a version tag.

- [ ] `VERSION`, `CMakeLists.txt` `project(... VERSION ...)`, and `include/simple-metricd/version.hpp` match
- [ ] [CHANGELOG.md](../CHANGELOG.md) has a dated section for the release
- [ ] [project/PROGRESS_REPORT.md](PROGRESS_REPORT.md) reflects what actually ships
- [ ] `cmake -B build -DENABLE_TESTS=ON && cmake --build build && ctest --test-dir build`
- [ ] `./build/simple-metricd --version` and `--test-config` succeed
- [ ] `./build/metricctl --config config/templates/development.conf list` succeeds
- [ ] Tag annotated: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`
- [ ] GitHub Release created from the tag
