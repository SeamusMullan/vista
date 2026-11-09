{
  lib,
  stdenv,
  src,
  cmake,
  pkg-config,
  makeWrapper,
  sdl3,
  sdl3-image,
  sdl3-ttf,
  openssl,
  feh,
  # OpenGL renderer (CMake option USE_SHADERS). Built in; enabled at runtime with `use_shaders = true`.
  withShaders ? true,
  libGL,
  glew,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "vista";
  version = "0.0.1"; # keep in sync with project(vista VERSION …) in CMakeLists.txt
  inherit src;

  nativeBuildInputs = [
    cmake
    pkg-config
    makeWrapper
  ];

  # USE_SYSTEM_SDL=ON → nixpkgs SDL3 via pkg-config. The git submodules are not needed and not fetched.
  buildInputs = [
    sdl3
    sdl3-image
    sdl3-ttf
    openssl # OpenSSL::Crypto (thumbnail cache hashing)
  ]
  ++ lib.optionals withShaders [
    libGL
    glew
  ];

  # Nix builds are sandboxed and reproducible; -march=native is neither.
  postPatch = ''
    substituteInPlace CMakeLists.txt --replace-quiet "-march=native" ""
  '';

  cmakeFlags = [
    (lib.cmakeBool "USE_SYSTEM_SDL" true)
    (lib.cmakeBool "USE_SHADERS" withShaders)
    (lib.cmakeBool "BUILD_TESTS" true)
    (lib.cmakeBool "BUILD_DOCS" false)
  ];

  # tests/test_config.c writes to /tmp — present in the sandbox.
  doCheck = true;

  # vista runs `feh …` through sh -c. Put feh on PATH regardless of the user's profile.
  postInstall = ''
    wrapProgram $out/bin/vista --prefix PATH : ${lib.makeBinPath [ feh ]}
  '';

  meta = {
    description = "dmenu-style wallpaper switcher with thumbnails, roulette mode and OpenRGB sync";
    homepage = "https://github.com/SeamusMullan/vista";
    license = lib.licenses.mit; # TODO match LICENSE
    platforms = lib.platforms.linux;
    mainProgram = "vista";
  };
})
