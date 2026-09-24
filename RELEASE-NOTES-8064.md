# R1Q2v2 Build 8064

Fixes client input behavior that could trigger legacy Rocket Arena's bot warning while Quake II ran in the background during Discord or desktop use.

Download `R1Q2v2-8064-win32.zip`. Quit Quake II, back up your existing binaries, and extract the archive into your Quake II directory. Launch `R1Q2v2.exe` and check for **Build 8064**. The package includes the client, dedicated server, game DLL, both R1GL renderer filenames, and required audio/network/image runtime DLLs. Your configurations and game data are not replaced. Retail / Steam game data is not included.

## Changes

- Remove desktop cursor sampling while the game does not own mouse input, including the resulting synthetic `BUTTON_ANY` flag.
- Preserve current command angles even when an input sample has zero elapsed time, including immediately after focus returns. Previously this path could send a zeroed angle pair between valid commands.
- Remove background-only packet clock caps. They discarded elapsed time and could prevent the connection's 100 ms send threshold from ever being reached. In synchronous mode, fractional frame intervals could also stall sending.
- Bound long command times to 250 ms instead of substituting a nominal `cl_maxfps` interval. Ordinary focused input and the existing first-sample discard after mouse reacquisition remain in place.

The screenshot's exact warning appears in [legacy Arena's `move_to_arena`](https://github.com/packetflinger/ra2/blob/main/arena.c). Its [client check](https://github.com/packetflinger/ra2/blob/main/p_client.c) flags repeated alternating pitch/yaw pairs; it is not the q2admin timing check assumed in the earlier release notes. These changes correct client input and clock behavior; they do not disable server checks. A server that already flagged the connection may require a reconnect.

## Validation

The Windows x86 Release build completed, and `tests/test-background-input.ps1` passed all 24 checks: both input modes, zero-time samples, foreground look, focus-return sample discard, buttons, and bounded command time. The preceding code failed 13 checks and reproduced Arena's angle flag. These regression tests now run in the Windows CI workflow.

The reporting player indicated that the issue appeared resolved after installing Build 8064. This is initial user confirmation; the separate automated runtime smoke test did not reach a usable session.

Credits: r1ch (original R1Q2), h0s3r (R1Q2v2).
