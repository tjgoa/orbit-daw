# Dependency record

This file records the upstream source revisions used for the current Orbit foundation. Update it when deliberately upgrading a dependency; don't silently follow a moving branch.

| Dependency | Upstream | Revision in this checkout | License path used |
|---|---|---|---|
| Tracktion Engine | <https://github.com/Tracktion/tracktion_engine> | `v3.2.0` (`0a5f4e6a5f53d09c89b414a44386a12df7fa1ec6`) | GPLv3; retain upstream notices |
| JUCE | <https://github.com/juce-framework/JUCE> | `19edd538429c93d277bf95b55aaa7e3eb545f951` (Tracktion Engine submodule) | AGPLv3; retain upstream notices |

The full dependency sources currently live under `Dependencies/tracktion_engine/` as a pinned Git submodule. Its checked-out commit is recorded by the Orbit repository, and the Tracktion Engine revision pins its JUCE submodule. Tracktion Engine's `.gitmodules` file uses an SSH URL for JUCE; the HTTPS rewrite in the README supports public checkouts without SSH keys.

Orbit application source is licensed under AGPLv3 in `LICENSE`. This record is not a replacement for reading each upstream license and all notices for included third-party components.
