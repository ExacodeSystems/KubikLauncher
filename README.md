<p align="center">
  <img alt="Kubik" src="https://cdn.kubik.gg/static/logo/kubik-lockup-transparent%402048x783.png" width="40%">
</p>

<p align="center">
  <b>KubikLauncher</b> is a Minecraft: Java Edition launcher from <a href="https://kubik.gg">kubik.gg</a>.<br />
  It sets up the mods your Kubik server needs, keeps them in step with the server, and joins it.
</p>

<p align="center">
  KubikLauncher is a <b>fork</b> of <a href="https://prismlauncher.org">Prism Launcher</a> and is <b>not</b> endorsed by or affiliated with the Prism Launcher project.
</p>

## What it does

KubikLauncher is a mod-first launcher built for servers hosted on kubik.gg, the Minecraft server hosting service run by PT Exacode Systems Technologies.

- **Online play only.** You sign in with the Microsoft account that owns Minecraft: Java Edition. There are no offline accounts, and the launcher never bypasses an authentication, ownership or licence check. Game files are downloaded only from Mojang's official servers.
- **Your Kubik servers in one place** *(in development)*. Sign in with your Kubik account to see the Java servers you run.
- **The right mods, automatically** *(in development)*. Pick a server, and KubikLauncher sets up the same Minecraft version and mod loader, installs the client-side mods the server needs from Modrinth, keeps them in sync when the server changes, and joins the server. Mods you add yourself are left alone.
- **Everything Prism Launcher does.** Separate instances, Fabric, Quilt, Forge and NeoForge, mod and modpack installs and updates from Modrinth, worlds, resource packs and shader packs.

## Status

KubikLauncher is in early development and has no public release yet. Releases will be published on this repository's [Releases](https://github.com/ExacodeSystems/KubikLauncher/releases) page and linked from [kubik.gg](https://kubik.gg).

## Support

- Bugs and feature requests: [GitHub issues](https://github.com/ExacodeSystems/KubikLauncher/issues).
- Questions about your Kubik server: [kubik.gg](https://kubik.gg) or halo@kubik.gg.

Please don't take KubikLauncher issues to the Prism Launcher project. They don't maintain this fork.

## Building

KubikLauncher builds like Prism Launcher. Follow Prism's [build instructions](https://prismlauncher.org/wiki/development/build-instructions) in this repository.

## Upstream and credits

KubikLauncher is built on the work of the [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher), PolyMC and MultiMC contributors. Their names are kept in the source and in the launcher's About dialog. The `develop` branch follows Prism Launcher's upstream.

Following Prism Launcher's forking policy, KubikLauncher ships its own API keys:

- Microsoft sign-in uses KubikLauncher's own Azure application. Prism Launcher's client ID is not used.
- The CurseForge API key is empty, so CurseForge features are switched off.

If you fork KubikLauncher, please use your own name, logo and API keys too.

## License [![GPL-3.0-only](https://img.shields.io/badge/License-GPL--3.0--only-C4282D?logo=gnu)](LICENSE)

All launcher code is available under the GPL-3.0-only license. Artwork that comes from Prism Launcher remains under the CC BY-SA 4.0 license (see [program_info/LICENSE](program_info/LICENSE)). The Kubik name and logo belong to PT Exacode Systems Technologies and are not covered by those licences.
