# Contributing to vista

Thanks for your interest in contributing to vista!

## Building

```bash
# Quick build
make

# Or with full options
./build.sh Release OFF

# With shaders
make shaders
```

## Code Style

- Follow existing code style (K&R-ish)
- Use 4 spaces for indentation
- Comment your code, especially complex algorithms
- Keep functions focused and under ~50 lines when possible
- Use meaningful variable names

## Documentation

- Add Doxygen comments to new functions
- Update man page if adding new options
- Regenerate docs: `make docs`

## Testing

Before submitting:
1. Build without warnings: `make debug`
2. Test on your wallpaper directory
3. Verify thumbnail caching works
4. Test with and without config file
5. Check memory leaks: `valgrind ./build/vista`

## Pull Requests

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit PR with clear description

## Feature Ideas

- Grid view mode
- Favorites/bookmarks
- Random wallpaper selection
- Multi-monitor support
- Video wallpaper preview
- Wayland support

## License

By contributing, you agree your code will be licensed under the same license as vista.
