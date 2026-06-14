Tracker (C++ port scaffold)
=================================

This folder contains a minimal C++ scaffold for the `Tracker` plugin, ported
from the C# `GH-Tracker` implementation. It's intended as a starting point to
complete a full port to the POEFixer C++ Plugin SDK.

Build
-----
Open `Tracker.vcxproj` in Visual Studio 2022 (MSVC v143 toolset), set the
configuration to `Release|x64` and build. The project expects the SDK headers
from the example `Radar` plugin under `Example/Radar/sdk` and `Example/Radar/imgui`.

Next steps
----------
- Port logic from `GH-Tracker/Tracker.cs` to `src/Tracker.cpp`.
- Implement rendering and settings import/export parity.
- Add any required assets (e.g. `status_icons.png`) into the project root and
  ensure they're copied to the plugin output folder.
