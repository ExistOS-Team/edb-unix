{ pkgs ? import <nixpkgs> {} }:
with pkgs;
stdenv.mkDerivation {
    name = "edb";
    src = ./.;
    builder = ./builder.sh;
    buildInputs = [
        cmake
    ];
}