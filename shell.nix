{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "elise-dev-env";

  # Build-time tools (nativeBuildInputs)
  # NOTE: 'gcc' removed here because mkShell provides it automatically
  nativeBuildInputs = with pkgs; [
    cmake
    pkg-config
    wayland            # Provides wayland-scanner tool
    wayland-scanner
  ];

  # Runtime and compilation dependencies
  buildInputs = with pkgs; [
    # FFmpeg & compression
    ffmpeg
    zlib

    # Graphics / OpenGL
    libGL
    libGLU

    # Audio backend
    alsa-lib

    # Wayland libraries
    wayland
    wayland-protocols
    libxkbcommon

    # X11 development libraries
    xorg.libX11
    xorg.libXrandr
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXi
    xorg.libXext
    xorg.libXrender
    xorg.libXfixes
  ];

  hardeningDisable = [ "pie" ];

  shellHook = ''
    export LD_LIBRARY_PATH="${pkgs.libGL}/lib:${pkgs.wayland}/lib:${pkgs.alsa-lib}/lib:${pkgs.xorg.libX11}/lib:$LD_LIBRARY_PATH"
    
    echo "========================================================"
    echo " 🛠️  ELISE Development Environment Loaded (with Wayland) 🛠️"
    echo "========================================================"
  '';
}