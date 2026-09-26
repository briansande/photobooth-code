# Reusable libraries

Put hardware-independent or reusable component logic here once it has been
proven in an isolated firmware project. Good candidates include shutter state
management, paper-feed movement, sensor debouncing, command parsing, and shared
status types.

Prefer non-blocking `update()` methods over long delays so the integrated
controller can operate several components concurrently. Give each library its
own directory with `library.json`, `src/`, and focused tests when appropriate.

[`PhotoboothWiFi/`](PhotoboothWiFi/README.md) is the shared Wi-Fi module. It
keeps the network credentials in one Git-ignored file for every firmware that
opts in.
