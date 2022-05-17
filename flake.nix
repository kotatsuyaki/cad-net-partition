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
          llvmPackages_14.bintools
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
        doc-deps = with pkgs; [
          texlive.combined.scheme-full
          pandoc
          librsvg
        ];
        fonts-conf = pkgs.makeFontsConf {
          fontDirectories = [ pkgs.source-han-serif ];
        };
      in
      {
        defaultPackage = self.packages."${system}".cad;
        packages.cad = cad;
        packages.docs = pkgs.stdenv.mkDerivation {
          name = "main.pdf";
          src = ./docs;
          buildInputs = doc-deps;
          buildPhase = "make";
          installPhase = ''
            mkdir -p $out
            cp main.pdf $out
          '';
          FONTCONFIG_FILE = fonts-conf;
        };
        devShell = with pkgs; mkShell {
          buildInputs = deps ++ dev-deps ++ doc-deps;
          CMAKE_EXPORT_COMPILE_COMMANDS = "yes";
        };
      });
}
