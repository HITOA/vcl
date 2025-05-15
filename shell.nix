with import <nixpkgs> {};
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [ 
    cmake
    gcc
    gdb
    pkg-config

    libllvm
    catch2_3
    doxygen
  ];

  NIX_LD_LIBRARY_PATH = lib.makeLibraryPath [
    stdenv.cc.cc
  ];

  NIX_LD = lib.fileContents "${stdenv.cc}/nix-support/dynamic-linker";
}