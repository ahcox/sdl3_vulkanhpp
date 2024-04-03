Some modular pieces of computer program code and applications using them, gathered into a single repository for reuse.

Most targets are disabled by default to make the project easier to build for the desired parts on new platforms.
You probably want to go into the cmake cache and turn on some options to get what you need.
For example, the option `MONOREPO_SDL3_CALLBACKS` controls building a simple SDL3 example using callbacks.