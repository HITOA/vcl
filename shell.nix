with import <nixpkgs> {};
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [ 
    cmake
    gdb
    pkg-config

    libllvm

    graphviz
  ];

  NIX_LD_LIBRARY_PATH = lib.makeLibraryPath [
    stdenv.cc.cc
  ];

  NIX_LD = lib.fileContents "${stdenv.cc}/nix-support/dynamic-linker";
}