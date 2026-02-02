{
  description = "FIRMUPS device-sdk environment";

  inputs.nixpkgs.url = "github:nixos/nixpkgs?ref=25.11";
  inputs.git-hooks = {
    url = "github:cachix/git-hooks.nix";
    inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    {
      self,
      nixpkgs,
      git-hooks,
    }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system: f system);
    in
    {
      checks = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };

          # Provides: clang-format, clang-tidy, run-clang-tidy
          clangTools = pkgs.llvmPackages.clang-tools;

          configureCMake = pkgs.writeShellScript "configure-cmake.sh" ''
            set -euo pipefail
            ${pkgs.cmake}/bin/cmake -S . -B $BUILD_DIR \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DCMAKE_BUILD_TYPE=Debug
          '';

          # This script expects filenames as args ("$@") and runs clang-tidy on them.
          clangTidyOnFiles = pkgs.writeShellScript "clang-tidy-on-files.sh" ''
            set -euo pipefail

            # Ensure compile_commands.json exists for clang-tidy to use.
            export BUILD_DIR="$(mktemp -d -t build-dir.XXXXXXXX)"
            ${configureCMake}

            # If no files were provided, do nothing.
            if [ "$#" -eq 0 ]; then
              exit 0
            fi

            clang-tidy -p $BUILD_DIR $(find src -type f \( -name '*.c' -o -name '*.h' \))
          '';

        in
        {
          pre-commit-check = git-hooks.lib.${system}.run {
            src = ./.;

            hooks = {
              nixfmt-rfc-style.enable = true;
              clang-format-check = {
                enable = true;
                name = "clang-format (check)";
                # clang-format "check" mode: fails if formatting would change
                entry = "${clangTools}/bin/clang-format --dry-run -Werror";
                language = "system";
                # Let pre-commit pass only matching filenames
                pass_filenames = true;
                files = "\\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$";
              };
              clang-tidy = {
                enable = true;
                name = "clang-tidy";
                entry = "${clangTidyOnFiles}";
                language = "system";
              };
              firmups-tests = {
                enable = true;
                name = "firmups-device-sdk tests";
                language = "system";
                pass_filenames = false;
                entry = "nix build -L --no-link .#firmups-device-sdk-test";
              };
            };
          };
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          preCommit = self.checks.${system}.pre-commit-check;
        in
        {
          default = pkgs.mkShell {
            name = "firmups-device-sdk";
            buildInputs = with pkgs; [
              gcc
              cmake
              clang
              ninja
              glibc
              glibc.static
              gdb
              valgrind
              bashInteractive
              nixfmt-rfc-style
            ];
            shellHook = ''
              # Enable git hooks
              ${preCommit.shellHook}
              export PS1="($name)$PS1"
              echo "Welcome to the $name devShell!"
            '';
          };
        }
      );
      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };

          tinycborSrc = pkgs.fetchgit {
            url = "https://github.com/intel/tinycbor.git";
            rev = "48a22bddfcc67b3a433ded695f906cc314a0bd5f";
            hash = "sha256-dHOEkQfFpDQDvutzeDwWgr9xFIJoBAbj7npuQXbymoI=";
          };

          asconSrc = pkgs.fetchgit {
            url = "https://github.com/ascon/ascon-c.git";
            rev = "9057124473a4b9cbcc8a028b65a4abf6b4222b0f";
            hash = "sha256-sqUdpP4ubRh/RuFsA+1+T8OIl0g+jjyffVZdbA+HjIg=";
          };

          unitySrc = pkgs.fetchgit {
            url = "https://github.com/ThrowTheSwitch/Unity.git";
            rev = "d89dafa41334ec4c55fafcf0b7cdb77a04148d43";
            hash = "sha256-50MbcFWrsXfGyLMqyc4QzWXUFJ3WBGKw1PVzqJDZLgU=";
          };

          injectDeps = ''
            echo "Injecting TinyCBOR into source tree..."
            mkdir -p "$sourceRoot/dependencies/tinycbor"
            cp -R --no-preserve=mode,ownership "${tinycborSrc}/." "$sourceRoot/dependencies/tinycbor/"

            echo "Injecting Ascon into source tree..."
            mkdir -p "$sourceRoot/dependencies/ascon-c"
            cp -R --no-preserve=mode,ownership "${asconSrc}/." "$sourceRoot/dependencies/ascon-c/"

            echo "Injecting Unity into source tree..."
            mkdir -p "$sourceRoot/dependencies/unity"
            cp -R --no-preserve=mode,ownership "${unitySrc}/." "$sourceRoot/dependencies/unity/"

            echo "$sourceRoot postUnpack complete."
          '';

        in
        {
          firmups-device-sdk-test = pkgs.stdenv.mkDerivation {
            pname = "firmups-device-sdk-test";
            version = "0.1.1";

            src = ./.;

            postUnpack = injectDeps;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
              "-DCMAKE_C_FLAGS=-fsanitize=address,undefined"
              "-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined"
              "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"
              "-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address,undefined"
              "-DFIRMUPS_TESTING=ON"
            ];

            doCheck = true;

            checkPhase = ''
              runHook preCheckPhase

              count="$(ctest -N 2>&1 || true)"
              echo "$count"

              if echo "$count" | grep -q 'No tests were found'; then
                echo "ERROR: CTest reports no tests were found. Failing."
                exit 1
              fi

              if echo "$count" | grep -qE 'Total Tests: *0'; then
                echo "ERROR: Total Tests: 0. Failing."
                exit 1
              fi

              export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
              export UBSAN_OPTIONS=print_stacktrace=1
              ctest --output-on-failure
              runHook postCheckPhase
            '';

            installPhase = ''
              runHook preInstallPhase
              mkdir -p $out
              runHook postInstallPhase
            '';
          };

          firmups-device-sdk = pkgs.stdenv.mkDerivation {
            pname = "firmups-device-sdk";
            version = "0.1.1";

            src = ./.;

            postUnpack = injectDeps;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config
            ];
            #buildInputs = [ pkgs.boost pkgs.openssl ]; # Add your deps here

            cmakeFlags = [
              "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
              "-DCMAKE_BUILD_TYPE=MinSizeRel"
            ];

            doCheck = false;

            installPhase = ''
              runHook preInstallPhase
              mkdir -p $out/lib
              mkdir -p $out/include
              # Copy the compiled library
              cp libfirmups-device-sdk.so $out/lib/

              # Copy headers
              cp -r $src/include/* $out/include/
              runHook postInstallPhase
            '';
          };
        }
      );

      apps = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {

          default = {
            type = "app";
            program = "${pkgs.writeShellScript "build-firmups-device-sdk" ''
              echo "Building firmups-device-sdk..."
              nix build .#firmups-device-sdk
            ''}";
          };
        }
      );

      formatter = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        pkgs.nixfmt-rfc-style
      );
    };
}
