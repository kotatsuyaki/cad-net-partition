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
          gcc
          gnumake
        ];
        dev-deps = with pkgs; [
          rnix-lsp
          bear
          clang-tools
          gdb
        ];
        cad = pkgs.stdenv.mkDerivation {
          name = "cad-pa-1";
          src = ./.;
          buildInputs = deps;
          installPhase = ''
            mkdir -p $out/bin
            cp pa1 $out/bin/cad-pa-1
          '';
        };
      in
      {
        packages.cad = cad;
        defaultPackage = self.packages."${system}".cad;
        devShell = with pkgs; mkShell {
          buildInputs = deps ++ dev-deps;
        };
      });
}
