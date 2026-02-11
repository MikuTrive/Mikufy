# Mikufy v2.11-nova
# 作者 luozenan - MikuTrive

{ pkgs ? import <nixpkgs> { system = "x86_64-linux"; },lib }:

pkgs.stdenv.mkDerivation rec {
  pname = "mikufy";
  version = "v2.11-nova";

src = lib.fileset.toSource {
            root = ./.;
            fileset = lib.fileset.unions [
              ./headers
              ./src
              ./web
              ./mikufy.desktop
            ];
          };

  nativeBuildInputs = with pkgs; [
            gcc
            pkg-config
          ];

          buildInputs = with pkgs; [
            webkitgtk_6_0
            gtk4
            file
            nlohmann_json
          ];

          buildPhase = ''
            # 编译主程序
            g++ -std=c++23 -O2 -Wall -Wextra -Wpedantic \
               $(pkg-config --cflags webkitgtk-6.0 gtk4) \
               $(pkg-config --cflags nlohmann_json) \
               -Iheaders \
               src/main.cpp \
               src/file_manager.cpp \
               src/web_server.cpp \
               src/window_manager.cpp \
               src/text_buffer.cpp \
               src/terminal_manager.cpp \
               src/process_launcher.cpp \
               src/terminal_window.cpp \
               -o mikufy \
               $(pkg-config --libs webkitgtk-6.0 gtk4) \
               -lmagic \
               $(pkg-config --libs nlohmann_json)

            # 编译 terminal_helper 独立程序
            g++ -std=c++23 -O2 -Wall -Wextra -Wpedantic \
               $(pkg-config --cflags webkitgtk-6.0 gtk4) \
               $(pkg-config --cflags nlohmann_json) \
               -Iheaders \
               src/terminal_helper.cpp \
               -o terminal_helper \
               $(pkg-config --libs webkitgtk-6.0 gtk4) \
               -lmagic \
               $(pkg-config --libs nlohmann_json)

          '';

          installPhase = ''
            mkdir -p $out/bin
            cp mikufy $out/bin/
            cp terminal_helper $out/bin/
            chmod +x $out/bin/mikufy
            chmod +x $out/bin/terminal_helper

            mkdir -p $out/share/mikufy/web
            cp -r ./web/* $out/share/mikufy/web/
            chmod -R 755 $out/share/mikufy/web

            mkdir -p $out/share/applications
            cp ./mikufy.desktop $out/share/applications/

            substituteInPlace $out/share/applications/mikufy.desktop --replace "Exec=mikufy" "Exec=$out/bin/mikufy"

            mkdir -p $out/share/icons/hicolor/256x256/apps
            cp ./web/Mikufy.png $out/share/icons/hicolor/256x256/apps/mikufy.png

          '';

  meta = with pkgs.lib; {
    description = "mikufy";
    homepage = "https://github.com/MikuTrive/Mikufy/tree/mikufy-v2.11-nova";
    license = licenses.gpl3;
    platforms = [ "x86_64-linux" ];
  };
}
