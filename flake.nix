{
  description = "CAD Programming Assignment 1";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.11";
    utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, utils, ... } @ inputs:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        deps = with pkgs; [
          clang_14
          gnumake
          cmake
        ];
        dev-deps = with pkgs; [
          rnix-lsp
          clang-tools
          gdb
        ];
        cad = pkgs.pkgsStatic.stdenv.mkDerivation {
          name = "cad-pa-2";
          src = ./.;
          buildInputs = deps;
          nativeBuildInputs = [ pkgs.cmake ];
          installPhase = ''
            mkdir -p $out/bin
            cp pa2 $out/bin/cad-pa-2
          '';
        };
      in
      {
        packages.cad = cad;
        defaultPackage = self.packages."${system}".cad;
        devShell = with pkgs; mkShell {
          buildInputs = deps ++ dev-deps;
          CMAKE_EXPORT_COMPILE_COMMANDS = "yes";
        };
      });
}
