========== GATE12 ==========
Copyright (C) 2025 Tilr

MacOS builds are untested and unsigned, please let me know of any issues by opening a ticket.
Because the builds are unsigned you may have to run the following commands:

sudo xattr -dr com.apple.quarantine /path/to/your/plugin/gate12.component
sudo xattr -dr com.apple.quarantine /path/to/your/plugin/gate12.vst3
sudo xattr -dr com.apple.quarantine /path/to/your/plugin/gate12.lv2

The command above will recursively remove the quarantine flag from the plugins.

