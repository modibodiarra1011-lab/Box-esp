# Component catalogue

Entries cataloguees: 258 families/profiles.

The catalogue distinguishes component identity from module-level wiring. Clone modules can change pins, voltage levels or interfaces, so a catalogue entry is not a guarantee that every breakout can connect directly to every board.

The Builder only uses an exact prepared recipe when a verified template exists. Otherwise it marks the result `REQUIRES_DRIVER`.

Add a component with:
`/COMPONENTS/MON_CAPTEUR/component.json` + `example.ino` + `wiring.md`, then rescan/rebuild the catalog using the provided tooling.
