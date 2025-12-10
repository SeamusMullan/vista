# Vista Installation Guide

Just run this:

```bash
./build.sh -i -s -r
```

That's it. Builds in release mode, enables shaders, and installs everything.

## Want to install to your home directory instead?

```bash
./build.sh -i -s -r -p $HOME/.local
```

Then make sure `~/.local/bin` is in your PATH:

```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

## Need more info?

Run `./build.sh -h` to see all the options.

## Setting up OpenRGB

Assuming your config already has it enabled. Just make sure OpenRGB is running a server thingy:

```bash
# Install it
sudo pacman -S openrgb

# Start the server
openrgb --server &
```

Now when you change wallpapers in Vista, your RGB peripherals will match the colors. Pretty cool eh?

If you wanna understand this more, read `build.sh` and maybe the code lol.
