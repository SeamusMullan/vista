{
  description = "vista — dmenu-style wallpaper switcher with thumbnails (SDL3, feh)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "aarch64-linux"
        "x86_64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      pkgsFor = system: nixpkgs.legacyPackages.${system};
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = pkgsFor system;
        in
        rec {
          vista = pkgs.callPackage ./nix/package.nix { src = self; };
          default = vista;
        }
      );

      # `nix develop` → everything needed to build + debug, plus feh for runtime testing.
      devShells = forAllSystems (
        system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.vista ];
            packages = with pkgs; [
              gdb
              valgrind
              clang-tools # clangd, clang-format
              doxygen
              feh
            ];
          };
        }
      );

      overlays.default = final: prev: {
        vista = final.callPackage ./nix/package.nix { src = self; };
      };

      formatter = forAllSystems (system: (pkgsFor system).nixfmt);
    };
}
