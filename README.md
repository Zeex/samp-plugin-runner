Plugin Runner
=============

[![Build status][build_badge_url]][build_url]

Plugin runner is a small program that can load and execute SA-MP server plugins
against a specified Pawn script. It can be used as a lightweight replacement
for the server, for example, when running automated plugin tests.

Usage
-----

```
plugin-runner path/to/plugin path/to/script.amx
```

Pass config options (fake `server.cfg`) with `--`:

```
plugin-runner path/to/plugin path/to/script.amx -- "port 8888" "long_call_time 2"
```

Note that no `--` will result in no `server.cfg` existing, `--` with no options
following will result in an empty `server.cfg` being generated, and options
being given will obviously be written to the file.

[build_url]: https://ci.appveyor.com/project/Zeex/plugin-runner/branch/master
[build_badge_url]: https://ci.appveyor.com/api/projects/status/qutulepfiep5y06i/branch/master?svg=true
