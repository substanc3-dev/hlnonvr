# hlnonvr
Run FPS-style Non-VR mods on top of Source2 (targets Half-Life: Alyx)

-----

 - Adds non-vr support to modern versions of the game
 - Adds support for loading mods using `-game`
 - Enables all convars (adds back `vr_enable_fake_vr_test` but also pretty nice for datamining schema stuff using `schema_` commands)
 - Support for all versions of the game (tested on 1.1, 1.2 and the preload non-vr supporting version)
 - Support for dedicated servers (using `-dedicated -allow_no_lobby_connect`)
 - Support for running mods in VR mode using `-vr`

# Usage:
 - Place this dll into `Half-Life Alyx\game\bin\win64`
 - Create a copy of "hlvr.exe" called whatever you would like (gonna be using "hlnonvr.exe" in this example) 
 - Open the executable ("hlnonvr.exe") in a hex editor
 - Find the string `engine2` (not the string `Failed to load the engine2 DLL` though) (in 1.2 it's at 0x10FB8)
 - Replace (don't add any bytes, overwrite existing ones) the string `engine2` with `hlnonvr`
 - You can now use this executable to run HLA (or mods using `-game`) in nonvr mode (no `-nonvr` flag or similar needed)

