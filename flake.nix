{
  description = "FIRMUPS device-sdk environment";

  inputs.nixpkgs.url = "github:nixos/nixpkgs?ref=25.05";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system:
        f system
      );
    in {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in {
          default = pkgs.mkShell {
            name = "firmups-device-sdk";
            buildInputs = with pkgs; [
              gcc14
              cmake
              llvmPackages_20.libcxxClang
              ninja
              glibc
              glibc.static
              gdb
              valgrind
              bashInteractive
              python313
              python313Packages.pip
            ];
            shellHook = ''
              export PS1="($name)$PS1"
              echo "Welcome to the $name devShell!"
            '';
          };
        }
      );
      packages = forAllSystems (system:
        let pkgs = import nixpkgs { inherit system; };
        in {
          firmups-device-sdk = pkgs.stdenv.mkDerivation {
            pname = "firmups-device-sdk";
            version = "0.1";

            src = ./.;

            nativeBuildInputs = [ pkgs.cmake pkgs.pkg-config ];
            #buildInputs = [ pkgs.boost pkgs.openssl ]; # Add your deps here

            #cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

            cmakeFlags = [
            #   "-DCMAKE_INSTALL_PREFIX=$out" 
              "-DCMAKE_C_FLAGS=-fsanitize=address,undefined"
              "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"
              "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
            ];

            doCheck = true;
            buildPhase = ''
              runHook preBuildPhase
              echo $src
              mkdir -p build
              cd build
              cmake $src
              make
              runHook postBuildPhase
            '';

            checkPhase = ''
              runHook preCheckPhase
              export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
              export UBSAN_OPTIONS=print_stacktrace=1
              ctest --output-on-failure
              runHook postCheckPhase
            '';
            
            # installPhase = ''
            #   mkdir -p $out/lib
            #   mkdir -p $out/include

            #   # Copy the compiled library
            #   cp path/to/build/libyourlib.so $out/lib/

            #   # Copy headers
            #   cp -r path/to/your/source/include/* $out/include/
            # '';
          };
        }
      );

      apps = forAllSystems (system: 
        let pkgs = import nixpkgs { inherit system; };
        in {

          default = {
            type = "app";
            program = "${pkgs.writeShellScript "build-firmups-device-sdk" ''
              echo "Building firmups-device-sdk..."
              nix build .#firmups-device-sdk
            ''}";
          };
        }
      );
    };
}
