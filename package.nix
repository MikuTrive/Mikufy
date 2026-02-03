# Mikufy v2.2(stable)
# 作者 luozenan

{ pkgs ? import <nixpkgs> { system = "x86_64-linux"; },lib }:

pkgs.stdenv.mkDerivation rec {
  pname = "mikufy";
  version = "2.2";

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
            g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic $(pkg-config --cflags webkitgtk-6.0 gtk4) $(pkg-config --cflags nlohmann_json) -Iheaders src/main.cpp src/file_manager.cpp src/web_server.cpp src/window_manager.cpp -o mikufy $(pkg-config --libs webkitgtk-6.0 gtk4) -lmagic $(pkg-config --libs nlohmann_json)
  
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp mikufy $out/bin/
            chmod +x $out/bin/mikufy

            mkdir -p $out/bin/web
            cp -r ./web/* $out/bin/web/
            chmod -R 755 $out/bin/web

            mkdir -p $out/share/applications
            cp ./mikufy.desktop $out/share/applications/

            substituteInPlace $out/share/applications/mikufy.desktop --replace "Exec=mikufy" "Exec=$out/bin/mikufy"

            mkdir -p $out/share/icons/hicolor/256x256/apps
            cp ./web/Mikufy.png $out/share/icons/hicolor/256x256/apps/mikufy.png
            
          '';

  meta = with pkgs.lib; {
    description = "mikufy";
    homepage = "https://github.com/MikuTrive/Mikufy";
    license = licenses.gpl3;
    platforms = [ "x86_64-linux" ];
  };
}
